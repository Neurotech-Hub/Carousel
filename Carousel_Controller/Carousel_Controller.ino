#include <Servo.h>
#include <AccelStepper.h>

#define STEP_PIN 7
#define DIR_PIN 8
#define ENABLE_PIN 4
#define MAG1_PIN 9      // Top Magnet
#define MAG2_PIN 10     // Bottom Magnet
#define SERVO1_PIN 11   // Right Door
#define SERVO2_PIN 12   // Left Door  
#define BEAM_S1_PIN A0  // Mainchamber side (inside)
#define BEAM_S2_PIN A1  // Subchamber side (outside)

// Version
const String VERSION = "1.3.0";

// Motor control parameters
float targetStepsPerSecond = 0;  // Target speed in steps/second
float maxAcceleration = 50;      // Gentle acceleration (steps/second²)

// Motor configuration parameters (default values)
float rmsCurrent = 3.2;       // RMS current setting (A)
int pulsePerRev = 800;        // Pulses per revolution from driver setting
bool halfCurrentMode = false; // Half current when stopped
int targetRPM = 10;           // Target RPM speed
int homingRPM = 8;            // Homing RPM speed
bool motorConfigured = true;  // Auto-configured with defaults

// Calibration data
int magnetIntervals[12];          // Step intervals: [0]=p1→p2, [1]=p2→p3, ..., [11]=p12→p1
int currentPosition = 0;          // 0=uncalibrated, 1-12=calibrated position
bool isCalibrated = false;        // Calibration status flag
long currentStepPosition = 0;     // Track absolute stepper position for navigation

// System state variables
bool waitingForCommand = true;
bool isDoorCycleArmed = false;     // "Arms" the door cycle for the next magnet-induced stop

// Beam breaker constants and state
const int BEAM_THRESHOLD = 700;

enum BeamMonitorState
{
  BEAM_IDLE,
  BEAM_ENTRY_STARTED,
  BEAM_MOUSE_IN_SUBCHAMBER,
  BEAM_EXIT_STARTED
};
BeamMonitorState beamState = BEAM_IDLE;
bool beamMonitoringActive = false;

// Serial command buffer for non-blocking reading
String commandBuffer = "";
const int MAX_COMMAND_LENGTH = 64;  // Maximum command length

// Magnet state machine
enum MagnetState
{
  UNKNOWN,
  ON_MAGNET,
  LEAVING_MAGNET, // Moving off the current magnet
  SEEKING_MAGNET  // Looking for the next magnet
};
MagnetState magnetState = UNKNOWN;

// Servo objects
Servo servo1;
Servo servo2;

// AccelStepper object (STEP/DIR interface)
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup()
{
  // Configure motor pins
  pinMode(ENABLE_PIN, OUTPUT);

  // Configure sensor pin
  pinMode(MAG2_PIN, INPUT_PULLUP);
  pinMode(MAG1_PIN, INPUT_PULLUP);
  
  // Configure beam breaker pins
  pinMode(BEAM_S1_PIN, INPUT_PULLUP);
  pinMode(BEAM_S2_PIN, INPUT_PULLUP);

  // Attach servos and set initial position
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(90); // Start in closed position
  servo2.write(90); // Start in closed position

  // Initialize AccelStepper
  digitalWrite(ENABLE_PIN, LOW); // Enable driver
  stepper.setMaxSpeed(200);     // Maximum possible speed (steps/second)
  stepper.setAcceleration(maxAcceleration); // Gentle acceleration
  stepper.setSpeed(0);           // Start stopped

  Serial.begin(115200);
  Serial.print("=== Carousel Controller ");
  Serial.print(VERSION);
  Serial.println(" ===");
  Serial.println("Commands:");
  Serial.println("  'setup,[RMS_current],[full_current],[pulse/rev],[RPM]' - Configure motor (optional)");
  Serial.println("    Example: setup,3.2,off,800,10");
  Serial.println("  'home' - Find home position (MAG1) and initialize system");
  Serial.println("  'p1' to 'p12' - Move to specific position (shortest path)");
  Serial.println("  'open' - Open door (only when on magnet)");
  Serial.println("  'close' - Close door (only when on magnet)");
  Serial.println("  'stop' or 's' - Emergency stop");
  Serial.println("  'rpm [value]' - Set target RPM (1-30)");
  Serial.println("  'mag' - Test magnetic sensor reading");
  Serial.println("  'beam' - Test beam breaker sensors");
  Serial.println("  'status' - Show system status");
  Serial.println();
  Serial.println("=== DEFAULT CONFIGURATION ===");
  Serial.print("RMS Current: ");
  Serial.print(rmsCurrent, 1);
  Serial.println("A");
  Serial.print("Half Current Mode: ");
  Serial.println(halfCurrentMode ? "ON" : "OFF");
  Serial.print("Pulses/Revolution: ");
  Serial.println(pulsePerRev);
  Serial.print("Target RPM: ");
  Serial.println(targetRPM);
  Serial.println();
  Serial.println("⚠️  IMPORTANT: Run 'home' command first to initialize the system!");
  Serial.println("Status: NOT HOMED");

  // Calculate default speed settings
  updateMotorSpeed();

  delay(500);
}

void loop()
{
  // Handle serial commands (non-blocking)
  handleCommands();

  // Run the stepper - this handles all acceleration/deceleration automatically
  // This must be called frequently for smooth motor operation
  if (motorConfigured)
  {
    stepper.run();
  }
  
  // Handle beam break monitoring if active
  if (beamMonitoringActive)
  {
    handleBeamMonitoring();
  }
}

void handleCommands()
{
  // Non-blocking serial reading - read one character at a time
  while (Serial.available() > 0)
  {
    char incomingChar = Serial.read();
    
    // Check for newline (command complete)
    if (incomingChar == '\n' || incomingChar == '\r')
    {
      // Only process if we have a command
      if (commandBuffer.length() > 0)
      {
        processCommand(commandBuffer);
        commandBuffer = ""; // Clear buffer for next command
      }
    }
    else
    {
      // Add character to buffer (with overflow protection)
      if (commandBuffer.length() < MAX_COMMAND_LENGTH)
      {
        commandBuffer += incomingChar;
      }
      else
      {
        // Buffer overflow - discard and warn
        commandBuffer = "";
        Serial.println("ERROR: Command too long! Maximum 64 characters.");
      }
    }
  }
}

void processCommand(String command)
{
  command.trim();
  command.toLowerCase(); // Standardize command input

  if (command.startsWith("setup,"))
  {
    parseSetupCommand(command);
  }
  else if (command == "home")
  {
    if (!motorConfigured)
    {
      Serial.println("ERROR: Motor not configured! Use 'setup' command first.");
      return;
    }
    handleHomingCommand();
  }
  else if (command.startsWith("p") && command.length() >= 2)
  {
    // Handle position commands p1-p12
    if (beamMonitoringActive)
    {
      Serial.println("ERROR: Cannot move while beam monitoring is active. Close the door first.");
      return;
    }
    
    int position = command.substring(1).toInt();
    if (position >= 1 && position <= 12)
    {
      handlePositionCommand(position);
    }
    else
    {
      Serial.println("ERROR: Position must be p1 to p12");
    }
  }
  else if (command == "open")
  {
    if (magnetState == ON_MAGNET)
    {
      openDoor();
    }
    else
    {
      Serial.println("ERROR: Can only open door when on a magnet.");
    }
  }
  else if (command == "close")
  {
    if (magnetState == ON_MAGNET)
    {
      closeDoor();
    }
    else
    {
      Serial.println("ERROR: Can only close door when on a magnet.");
    }
  }
  else if (command == "stop" || command == "s")
  {
    isDoorCycleArmed = false; // Prevent door cycle on manual stop
    stopMotor();
    Serial.println("Motor stopped by user.");
    waitingForCommand = true;
  }
  else if (command.startsWith("rpm "))
  {
    if (!motorConfigured)
    {
      Serial.println("ERROR: Configure motor first with 'setup' command.");
      return;
    }
    int newRPM = command.substring(4).toInt();
    if (newRPM >= 1 && newRPM <= 30)
    {
      targetRPM = newRPM;
      
      float currentSpeed = stepper.speed();
      float currentRPM = (currentSpeed * 60.0) / pulsePerRev;
      Serial.print("Target RPM changed to ");
      Serial.print(targetRPM);
      Serial.print(" (Current: ");
      Serial.print(currentRPM, 1);
      Serial.println(" RPM)");

      // Use the hybrid approach for speed updates
      updateMotorSpeed();
    }
    else
    {
      Serial.println("ERROR: RPM must be between 1-30");
    }
  }
  else if (command == "mag")
  {
    testSensor();
  }
  else if (command == "beam")
  {
    testBeamSensors();
  }
  else if (command == "status")
  {
    printStatus();
  }
  else
  {
    Serial.println("Commands: home | p1-p12 | open | close | stop/s | rpm [value] | mag | beam | status | setup,[RMS_current],[full_current],[pulse/rev],[RPM]");
  }
}

void parseSetupCommand(String command)
{
  // Parse: setup,current,full_current,pulse_per_rev,rpm
  int commaCount = 0;
  String values[4];
  int startIndex = 6; // Skip "setup,"

  for (int i = startIndex; i < command.length(); i++)
  {
    if (command[i] == ',' || i == command.length() - 1)
    {
      if (i == command.length() - 1)
      {
        values[commaCount] = command.substring(startIndex, i + 1);
      }
      else
      {
        values[commaCount] = command.substring(startIndex, i);
      }
      startIndex = i + 1;
      commaCount++;
      if (commaCount >= 4)
        break;
    }
  }

  if (commaCount == 4)
  {
    rmsCurrent = values[0].toFloat();
    String halfCurrentVal = values[1];
    halfCurrentVal.trim();
    halfCurrentVal.toLowerCase();

    if (halfCurrentVal == "on")
    {
      halfCurrentMode = true;
    }
    else if (halfCurrentVal == "off")
    {
      halfCurrentMode = false;
    }
    else
    {
      Serial.println("ERROR: Invalid value for full_current. Use 'on' or 'off'.");
      return;
    }
    pulsePerRev = values[2].toInt();
    targetRPM = values[3].toInt();

    // Validate inputs
    if (rmsCurrent < 0.5 || rmsCurrent > 4.2)
    {
      Serial.println("ERROR: Current must be 0.5-4.2A");
      return;
    }
    if (pulsePerRev < 200 || pulsePerRev > 25600)
    {
      Serial.println("ERROR: Pulse/rev must be 200-25600");
      return;
    }
    if (targetRPM < 1 || targetRPM > 30)
    {
      Serial.println("ERROR: RPM must be 1-30");
      return;
    }

    updateMotorSpeed();
    motorConfigured = true;

    Serial.println("=== MOTOR CONFIGURED ===");
    Serial.print("RMS Current: ");
    Serial.print(rmsCurrent, 2);
    Serial.println("A");
    Serial.print("Half Current Mode: ");
    Serial.println(halfCurrentMode ? "ON" : "OFF");
    Serial.print("Pulses/Revolution: ");
    Serial.println(pulsePerRev);
    Serial.print("Target RPM: ");
    Serial.println(targetRPM);
    Serial.print("Calculated Target Speed: ");
    Serial.print(targetStepsPerSecond, 1);
    Serial.println(" steps/sec");
    Serial.println("Motor ready! Use 'init' to begin.");
  }
  else
  {
    Serial.println("ERROR: Format = setup,[RMS_current],[full_current(on/off)],[pulse/rev],[RPM]");
    Serial.println("Example: setup,3.0,off,800,60");
  }
}

void calculateOptimalSpeed()
{
  // Calculate target speed in steps per second
  targetStepsPerSecond = (float(targetRPM) * float(pulsePerRev)) / 60.0;
  
  // Safety limits for speed
  float minStepsPerSecond = (1.0 * pulsePerRev) / 60.0;  // 1 RPM minimum
  float maxStepsPerSecond = (30.0 * pulsePerRev) / 60.0; // 30 RPM maximum
  
  if (targetStepsPerSecond < minStepsPerSecond)
    targetStepsPerSecond = minStepsPerSecond;
  if (targetStepsPerSecond > maxStepsPerSecond)
    targetStepsPerSecond = maxStepsPerSecond;
}

void updateMotorSpeed()
{
  // Calculate the target speed first
  calculateOptimalSpeed();
  
  if (stepper.isRunning())
  {
    // Motor is running - use setSpeed for dynamic speed change
    stepper.setSpeed(targetStepsPerSecond);
    Serial.print("Speed dynamically adjusted to ");
    Serial.print(targetStepsPerSecond, 1);
    Serial.println(" steps/sec");
  }
  else
  {
    // Motor is stopped - safe to update maxSpeed for next movement
    stepper.setMaxSpeed(targetStepsPerSecond);
    Serial.print("Target speed set to ");
    Serial.print(targetStepsPerSecond, 1);
    Serial.println(" steps/sec for next movement");
  }
}

void stopMotor()
{
  stepper.stop();  // AccelStepper handles smooth deceleration
  waitingForCommand = true;
}

// Servo control functions with speed control
void moveServoSlow(Servo &servo, int targetPosition)
{
  int currentPosition = servo.read();
  int direction = (targetPosition > currentPosition) ? 1 : -1;
  
  for (int pos = currentPosition; pos != targetPosition; pos += direction * 10)
  {
    // Make sure we don't overshoot
    if ((direction == 1 && pos > targetPosition) || (direction == -1 && pos < targetPosition))
    {
      pos = targetPosition;
    }
    servo.write(pos);
    delay(20); // 20ms delay between 10-degree steps
    
    if (pos == targetPosition) break;
  }
}

void openDoor()
{
  Serial.println("Opening door...");
  moveServoSlow(servo1, 0);   // servo1 to 0 degrees
  moveServoSlow(servo2, 180); // servo2 to 180 degrees
}

void closeDoor()
{
  stopBeamMonitoring(); // Stop monitoring if active
  Serial.println("Closing door...");
  moveServoSlow(servo1, 90);  // servo1 to 90 degrees
  moveServoSlow(servo2, 90);  // servo2 to 90 degrees
}

void automaticDoorCycle()
{
  openDoor();
  startBeamMonitoring();
}

void testSensor()
{
  Serial.println("Testing sensors for 10 seconds...");
  unsigned long startTime = millis();

  while (millis() - startTime < 10000)
  {
    int mag1Value = digitalRead(MAG1_PIN);
    int mag2Value = digitalRead(MAG2_PIN);

    if (mag1Value == LOW)
    {
      Serial.println("Home sensor detected!");
    }
    else if (mag2Value == LOW)
    {
      Serial.println("Position sensor detected!");
    }
    else
    {
      Serial.println("No sensor detected");
    }
    delay(300);
  }
  Serial.println("Test complete.");
}

// Beam breaker sensor functions
bool isBeamBroken(int pin)
{
  // Single read for mouse detection
  int value = analogRead(pin);
  return (value > BEAM_THRESHOLD);
}

void handleBeamMonitoring()
{
  // Non-blocking state machine for beam break sequence detection
  bool s1Broken = isBeamBroken(BEAM_S1_PIN);
  bool s2Broken = isBeamBroken(BEAM_S2_PIN);
  
  switch (beamState)
  {
    case BEAM_IDLE:
      // Waiting for mouse to start entering (S1 should break first)
      if (s1Broken)
      {
        beamState = BEAM_ENTRY_STARTED;
        // Silent - don't print intermediate state
      }
      break;
      
    case BEAM_ENTRY_STARTED:
      // S1 was broken, now waiting for S2 to break (mouse fully entering)
      if (s2Broken)
      {
        beamState = BEAM_MOUSE_IN_SUBCHAMBER;
        Serial.println("Mouse in subchamber");
      }
      // If S1 clears without S2 breaking, ignore (mouse backed out)
      break;
      
    case BEAM_MOUSE_IN_SUBCHAMBER:
      // Mouse is in subchamber, waiting for exit sequence to start (S2 breaks first)
      if (s2Broken)
      {
        beamState = BEAM_EXIT_STARTED;
        // Silent - don't print intermediate state
      }
      break;
      
    case BEAM_EXIT_STARTED:
      // S2 was broken (mouse exiting), waiting for S1 to break (mouse back in mainchamber)
      if (s1Broken)
      {
        Serial.println("Mouse returned - closing door");
        closeDoor(); // This will also stop beam monitoring
      }
      // If S2 clears without S1 breaking, ignore (mouse backed out)
      break;
  }
}

void startBeamMonitoring()
{
  beamState = BEAM_IDLE;
  beamMonitoringActive = true;
  Serial.println("Beam monitoring active - waiting for mouse movement");
}

void stopBeamMonitoring()
{
  beamMonitoringActive = false;
  beamState = BEAM_IDLE;
}

void testBeamSensors()
{
  Serial.println("Testing beam breaker sensors for 10 seconds...");
  Serial.println("Format: S1(mainchamber) | S2(subchamber)");
  unsigned long startTime = millis();

  while (millis() - startTime < 10000)
  {
    int s1Value = analogRead(BEAM_S1_PIN);
    int s2Value = analogRead(BEAM_S2_PIN);
    bool s1Blocked = (s1Value > BEAM_THRESHOLD);
    bool s2Blocked = (s2Value > BEAM_THRESHOLD);
    
    Serial.print("S1=");
    Serial.print(s1Value);
    Serial.print(" ");
    Serial.print(s1Blocked ? "BLOCKED" : "CLEAR");
    Serial.print("  |  S2=");
    Serial.print(s2Value);
    Serial.print(" ");
    Serial.println(s2Blocked ? "BLOCKED" : "CLEAR");
    delay(100);
  }
  Serial.println("Test complete.");
}

void printStatus()
{
  Serial.println("\n=== SYSTEM STATUS ===");
  Serial.print("Version: ");
  Serial.println(VERSION);
  
  Serial.print("Motor Configured: ");
  Serial.println(motorConfigured ? "YES" : "NO");

  if (motorConfigured)
  {
    Serial.print("RMS Current: ");
    Serial.print(rmsCurrent, 2);
    Serial.println("A");
    Serial.print("Pulses/Rev: ");
    Serial.println(pulsePerRev);
    Serial.print("Target RPM: ");
    Serial.println(targetRPM);
    Serial.print("Target Speed (steps/sec): ");
    Serial.print(targetStepsPerSecond, 1);
    Serial.println(" steps/sec");
  }
  
  Serial.println();
  Serial.print("Homing Status: ");
  if (isCalibrated)
  {
    Serial.println("HOMED ✓");
    Serial.print("Current Position: p");
    Serial.println(currentPosition);
  }
  else
  {
    Serial.println("NOT HOMED ⚠️");
    Serial.println("Run 'home' command to initialize the system.");
  }

  Serial.print("Motor State: ");
  if (stepper.isRunning())
  {
    Serial.print("MOVING");
  }
  else
  {
    Serial.print("STOPPED");
  }
  Serial.println();

  if (stepper.isRunning())
  {
    float currentSpeed = stepper.speed();
    float currentRPM = (currentSpeed * 60.0) / pulsePerRev;
    Serial.print("Current RPM: ");
    Serial.print(currentRPM, 1);
    Serial.print(" | Current Speed: ");
    Serial.print(currentSpeed, 1);
    Serial.println(" steps/sec");
  }

  Serial.print("Direction: ");
  if (stepper.speed() > 0)
    Serial.println("Forward");
  else if (stepper.speed() < 0)
    Serial.println("Reverse");
  else
    Serial.println("Stopped");

  Serial.print("Stepper Position: ");
  Serial.println(stepper.currentPosition());

  Serial.print("Home Sensor: ");
  Serial.println(digitalRead(MAG1_PIN) == LOW ? "DETECTED" : "Clear");
  Serial.print("Position Sensor: ");
  Serial.println(digitalRead(MAG2_PIN) == LOW ? "DETECTED" : "Clear");
  
  Serial.print("Magnet State: ");
  switch (magnetState)
  {
  case UNKNOWN:
    Serial.println("UNKNOWN");
    break;
  case ON_MAGNET:
    Serial.println("ON_MAGNET");
    break;
  case LEAVING_MAGNET:
    Serial.println("LEAVING_MAGNET");
    break;
  case SEEKING_MAGNET:
    Serial.println("SEEKING_MAGNET");
    break;
  }
  
  Serial.println();
  Serial.print("Beam Monitoring: ");
  Serial.println(beamMonitoringActive ? "ACTIVE" : "INACTIVE");
  
  if (beamMonitoringActive)
  {
    Serial.print("Beam State: ");
    switch (beamState)
    {
    case BEAM_IDLE:
      Serial.println("IDLE (waiting for S1)");
      break;
    case BEAM_ENTRY_STARTED:
      Serial.println("ENTRY_STARTED (S1 detected, waiting for S2)");
      break;
    case BEAM_MOUSE_IN_SUBCHAMBER:
      Serial.println("MOUSE_IN_SUBCHAMBER (waiting for S2 exit)");
      break;
    case BEAM_EXIT_STARTED:
      Serial.println("EXIT_STARTED (S2 detected, waiting for S1)");
      break;
    }
  }
  
  // Show current beam sensor readings
  int s1Value = analogRead(BEAM_S1_PIN);
  int s2Value = analogRead(BEAM_S2_PIN);
  bool s1Blocked = (s1Value > BEAM_THRESHOLD);
  bool s2Blocked = (s2Value > BEAM_THRESHOLD);
  
  Serial.print("Beam S1 (mainchamber): ");
  Serial.print(s1Value);
  Serial.print(" - ");
  Serial.println(s1Blocked ? "BLOCKED" : "CLEAR");
  
  Serial.print("Beam S2 (subchamber): ");
  Serial.print(s2Value);
  Serial.print(" - ");
  Serial.println(s2Blocked ? "BLOCKED" : "CLEAR");

  Serial.println("=====================\n");
}

void handleHomingCommand()
{
  Serial.println("\n=== HOMING START ===");
  Serial.println("Finding Home Position...");
  
  // Reset homing data
  isCalibrated = false;
  currentPosition = 0;
  magnetState = UNKNOWN;
  
  // Set homing speed (8 RPM)
  int savedRPM = targetRPM;
  targetRPM = homingRPM;
  calculateOptimalSpeed();
  stepper.setMaxSpeed(targetStepsPerSecond);
  
  // Search for MAG1
  stepper.setCurrentPosition(0);
  currentStepPosition = 0;
  
  if (digitalRead(MAG1_PIN) == LOW)
  {
    Serial.println("Already at home position.");
  }
  else
  {
    Serial.println("Searching for home position...");
    // Move until MAG1 found
    stepper.setSpeed(targetStepsPerSecond);
    stepper.move(100000); // Large number
    
    while (digitalRead(MAG1_PIN) != LOW)
    {
      stepper.run();
    }
    
    // Stop gently at MAG1
    long currentPos = stepper.currentPosition();
    stepper.moveTo(currentPos);
    while (stepper.run()) {} // Wait for stop
    
    Serial.println("Home position found.");
  }
  
  // Reset position counter at MAG1
  stepper.setCurrentPosition(0);
  currentStepPosition = 0;
  
  // Fill magnetIntervals array with hardcoded value of 840 steps
  for (int i = 0; i < 12; i++)
  {
    magnetIntervals[i] = 840;
  }
  
  // Restore original RPM
  targetRPM = savedRPM;
  calculateOptimalSpeed();
  
  // Set homing complete
  isCalibrated = true;
  currentPosition = 1; // We're at MAG1 (p1)
  magnetState = ON_MAGNET;

  Serial.println("\n=== HOMING COMPLETE ===");
  Serial.println("System initialized with 842 steps between positions.");
  Serial.println("You can now use p1-p12 commands.");
  Serial.println("Currently at Home Position (Box 1)");
}

void handlePositionCommand(int targetPos)
{
  if (!isCalibrated)
  {
    Serial.println("ERROR: System not homed! Run 'home' command first.");
    return;
  }
  
  if (currentPosition == 0)
  {
    Serial.println("ERROR: Current position unknown! Run 'home' command.");
    return;
  }
  
  if (targetPos == currentPosition)
  {
    Serial.print("Already at position p");
    Serial.println(targetPos);
    return;
  }
  
  // Calculate shortest path
  int stepsForward = (targetPos - currentPosition + 12) % 12;
  int stepsBackward = (currentPosition - targetPos + 12) % 12;
  
  if (stepsForward == 0) stepsForward = 12;
  if (stepsBackward == 0) stepsBackward = 12;
  
  bool goForward = (stepsForward <= stepsBackward);
  int positionsToMove = goForward ? stepsForward : stepsBackward;
  
  // Calculate total steps
  long totalSteps = 0;
  if (goForward)
  {
    for (int i = 0; i < positionsToMove; i++)
    {
      int intervalIndex = (currentPosition - 1 + i) % 12;
      totalSteps += magnetIntervals[intervalIndex];
    }
  }
  else
  {
    for (int i = 0; i < positionsToMove; i++)
    {
      int intervalIndex = (currentPosition - 2 - i + 12) % 12;
      totalSteps -= magnetIntervals[intervalIndex];
    }
  }
  
  // Execute movement
  Serial.print("Moving from p");
  Serial.print(currentPosition);
  Serial.print(" to p");
  Serial.print(targetPos);
  Serial.print(" (");
  Serial.print(goForward ? "forward" : "backward");
  Serial.print(", ");
  Serial.print(abs(totalSteps));
  Serial.println(" steps)");
  
  // Set target position and start movement
  isDoorCycleArmed = true;
  waitingForCommand = false;
  
  long targetStepPosition = currentStepPosition + totalSteps;
  stepper.moveTo(targetStepPosition);
  
  // Wait for movement to complete
  while (stepper.run())
  {
    // Optional: add progress reporting here
  }
  
  // Update position tracking
  currentStepPosition = targetStepPosition;
  currentPosition = targetPos;
  
  // Verify magnet detection
  bool mag1 = (digitalRead(MAG1_PIN) == LOW);
  bool mag2 = (digitalRead(MAG2_PIN) == LOW);
  
  if (mag1 || mag2)
  {
    magnetState = ON_MAGNET;
    Serial.print("Arrived at Box ");
    Serial.print(targetPos);
    Serial.println(" - Sensor detected ✓");
    
    waitingForCommand = true;
    
    // Trigger door cycle
    if (isDoorCycleArmed)
    {
      Serial.println("Starting door cycle...");
      automaticDoorCycle();
      isDoorCycleArmed = false;
    }
  }
  else
  {
    magnetState = UNKNOWN;
    Serial.println("ERROR: No magnet detected at target position!");
    Serial.println("Position may have drifted. Please run 'home' to re-home the system.");
    isCalibrated = false;
    currentPosition = 0;
  }
}
