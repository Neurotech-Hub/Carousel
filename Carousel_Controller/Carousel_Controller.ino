#include <Servo.h>
#include <AccelStepper.h>

#define STEP_PIN 7
#define DIR_PIN 8
#define ENABLE_PIN 4
#define MAG1_PIN 9
#define MAG2_PIN 10
#define SERVO1_PIN 11
#define SERVO2_PIN 12

// Version
const String VERSION = "v1.10";

// Motor control parameters
float targetStepsPerSecond = 0;  // Target speed in steps/second
float maxAcceleration = 50;      // Gentle acceleration (steps/second²)

// Motor configuration parameters (default values)
float rmsCurrent = 3.2;       // RMS current setting (A)
int pulsePerRev = 800;        // Pulses per revolution from driver setting
bool halfCurrentMode = false; // Half current when stopped
int targetRPM = 10;           // Target RPM speed
int calibrationRPM = 8;       // Calibration RPM speed
bool motorConfigured = true;  // Auto-configured with defaults

// Calibration data
int magnetIntervals[12];          // Step intervals: [0]=p1→p2, [1]=p2→p3, ..., [11]=p12→p1
int currentPosition = 0;          // 0=uncalibrated, 1-12=calibrated position
bool isCalibrated = false;        // Calibration status flag
long currentStepPosition = 0;     // Track absolute stepper position for navigation
int adjustmentSteps = 0;          // Adjustment Steps to feel discrepancy according to calibration speed

// System state variables
bool waitingForCommand = true;
bool isDoorCycleArmed = false;     // "Arms" the door cycle for the next magnet-induced stop

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
  Serial.println("  'cal' - Calibrate system (find MAG1, record all 12 positions)");
  Serial.println("  'p1' to 'p12' - Move to specific position (shortest path)");
  Serial.println("  'open' - Open door (only when on magnet)");
  Serial.println("  'close' - Close door (only when on magnet)");
  Serial.println("  'stop' or 's' - Emergency stop");
  Serial.println("  'rpm [value]' - Set target RPM (1-30)");
  Serial.println("  'mag' - Test sensor reading");
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
  Serial.println("⚠️  IMPORTANT: Run 'cal' command first to calibrate the system!");
  Serial.println("Status: NOT CALIBRATED");

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
  else if (command == "cal")
  {
    if (!motorConfigured)
    {
      Serial.println("ERROR: Motor not configured! Use 'setup' command first.");
      return;
    }
    handleCalibrationCommand();
  }
  else if (command.startsWith("p") && command.length() >= 2)
  {
    // Handle position commands p1-p12
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
  else if (command == "status")
  {
    printStatus();
  }
  else
  {
    Serial.println("Commands: cal | p1-p12 | open | close | stop/s | rpm [value] | mag | status | setup,[RMS_current],[full_current],[pulse/rev],[RPM]");
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
  Serial.println("Closing door...");
  moveServoSlow(servo1, 90);  // servo1 to 90 degrees
  moveServoSlow(servo2, 90);  // servo2 to 90 degrees
}

void automaticDoorCycle()
{
  openDoor();
  delay(1000); // Keep door open for 1 second
  closeDoor();
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
  Serial.print("Calibration Status: ");
  if (isCalibrated)
  {
    Serial.println("CALIBRATED ✓");
    Serial.print("Current Position: p");
    Serial.println(currentPosition);
  }
  else
  {
    Serial.println("NOT CALIBRATED ⚠️");
    Serial.println("Run 'cal' command to calibrate the system.");
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

  Serial.println("=====================\n");
}

void handleCalibrationCommand()
{
  Serial.println("\n=== CALIBRATION START ===");
  Serial.println("Finding Home Position...");
  
  // Reset calibration data
  isCalibrated = false;
  currentPosition = 0;
  magnetState = UNKNOWN;
  
  // Set calibration speed (8 RPM)
  int savedRPM = targetRPM;
  targetRPM = calibrationRPM;
  calculateOptimalSpeed();
  stepper.setMaxSpeed(targetStepsPerSecond);
  
  // Search for MAG1
  stepper.setCurrentPosition(0);
  currentStepPosition = 0;
  
  if (digitalRead(MAG1_PIN) == LOW)
  {
    Serial.println("Already at home position.");
    Serial.println("Starting full rotation to record intervals...");
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
    Serial.println("Starting full rotation to record intervals...");
  }
  
  // Reset position counter at MAG1
  stepper.setCurrentPosition(0);
  currentStepPosition = 0;
  long lastMagnetPosition = 0;
  int magnetCount = 0;
  
  // Move off MAG1
  stepper.move(100000);
  stepper.setSpeed(targetStepsPerSecond);
  while (digitalRead(MAG1_PIN) == LOW || digitalRead(MAG2_PIN) == LOW)
  {
    stepper.run();
  }
  
  Serial.println("Cleared home position, now recording positions...");
  
  // Record intervals for full rotation
  bool lastMag1State = false;
  bool lastMag2State = false;
  
  while (magnetCount < 12)
  {
    stepper.run();
    bool mag1Detected = (digitalRead(MAG1_PIN) == LOW);
    bool mag2Detected = (digitalRead(MAG2_PIN) == LOW);
    
    // Detect rising edge (entering magnet)
    if ((mag1Detected || mag2Detected) && (!lastMag1State && !lastMag2State))
    {
      long currentPos = stepper.currentPosition();
      magnetIntervals[magnetCount] = currentPos - lastMagnetPosition;
      
      Serial.print("Box ");
      Serial.print(magnetCount + 1);
      Serial.print(" detected at position ");
      Serial.print(currentPos);
      Serial.print(" (interval: ");
      Serial.print(magnetIntervals[magnetCount]);
      Serial.println(" steps)");
      
      lastMagnetPosition = currentPos;
      magnetCount++;
      
      // Wait to clear magnet
      while (digitalRead(MAG1_PIN) == LOW || digitalRead(MAG2_PIN) == LOW)
      {
        stepper.run();
      }
      
      // Break if we've found all 12
      if (magnetCount >= 12) break;
    }
    
    lastMag1State = mag1Detected;
    lastMag2State = mag2Detected;
  }
  
  // Stop motor
  long finalPos = stepper.currentPosition();
  adjustmentSteps = pulsePerRev * 0.05;
  stepper.moveTo(finalPos - adjustmentSteps);
  while (stepper.run()) {}
  
  // Restore original RPM
  targetRPM = savedRPM;
  calculateOptimalSpeed();
  
  if (magnetCount == 12)
  {
    isCalibrated = true;
    currentPosition = 1; // We're at MAG1 (p1)
    magnetState = ON_MAGNET;
    currentStepPosition = stepper.currentPosition(); // Sync logical position with actual position

    Serial.println("\n=== CALIBRATION COMPLETE ===");
    Serial.println("Recorded intervals (steps between magnets):");
    for (int i = 0; i < 12; i++)
    {
      Serial.print("  p");
      Serial.print(i + 1);
      Serial.print(" -> p");
      Serial.print((i + 1) % 12 + 1);
      Serial.print(": ");
      Serial.print(magnetIntervals[i]);
      Serial.println(" steps");
    }
    Serial.println("\nSystem calibrated! You can now use p1-p12 commands.");
    Serial.println("Currently at Home Position (Box 1)");
  }
  else
  {
    Serial.println("\n=== CALIBRATION FAILED ===");
    Serial.print("ERROR: Only found ");
    Serial.print(magnetCount);
    Serial.println(" magnets (expected 12)");
    Serial.println("Please check magnet installation and try again.");
    isCalibrated = false;
    currentPosition = 0;
  }
}

void handlePositionCommand(int targetPos)
{
  if (!isCalibrated)
  {
    Serial.println("ERROR: System not calibrated! Run 'cal' command first.");
    return;
  }
  
  if (currentPosition == 0)
  {
    Serial.println("ERROR: Current position unknown! Run 'cal' command.");
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
      Serial.println("Starting automatic door cycle...");
      automaticDoorCycle();
      isDoorCycleArmed = false;
    }
  }
  else
  {
    magnetState = UNKNOWN;
    Serial.println("ERROR: No magnet detected at target position!");
    Serial.println("Position may have drifted. Please run 'cal' to recalibrate.");
    isCalibrated = false;
    currentPosition = 0;
  }
}
