#include <Servo.h>
#include <AccelStepper.h>

#define STEP_PIN 7
#define DIR_PIN 8
#define ENABLE_PIN 4
#define MAG1_PIN 9
#define MAG2_PIN 10
#define SERVO1_PIN 11
#define SERVO2_PIN 12

// Motor control parameters
float targetStepsPerSecond = 0;  // Target speed in steps/second
float maxAcceleration = 75;      // Gentle acceleration (steps/second²)
float startingSpeed = 13.33;     // Starting speed ~1 RPM (for 800 pulse/rev)

// Motor configuration parameters (default values)
float rmsCurrent = 3.2;       // RMS current setting (A)
int pulsePerRev = 800;        // Pulses per revolution from driver setting
bool halfCurrentMode = false; // Half current when stopped
int targetRPM = 10;           // Target RPM speed
bool motorConfigured = true;  // Auto-configured with defaults

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
  Serial.println("=== Carousel Controller ===");
  Serial.println("Commands:");
  Serial.println("  'setup,[RMS_current],[full_current],[pulse/rev],[RPM]' - Configure motor (optional)");
  Serial.println("    Example: setup,0.71,off,800,60");
  Serial.println("  'init' - Initialize and align with first magnet");
  Serial.println("  'next' - Move to next magnet position");
  Serial.println("  'open' - Open door (only when on magnet)");
  Serial.println("  'close' - Close door (only when on magnet)");
  Serial.println("  'rpm [value]' - Set target RPM (1-30, works while running)");
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
  Serial.println("Ready to use! Send 'init' to begin or 'setup' to change configuration.");
  Serial.println("Magnet state: UNKNOWN");

  // Calculate default speed settings
  updateMotorSpeed();

  delay(500);
}

void loop()
{
  // Handle serial commands
  handleCommands();

  // Handle motor movement with AccelStepper
  if (motorConfigured)
  {
    bool isMag1Present = (digitalRead(MAG1_PIN) == LOW);
    bool isMag2Present = (digitalRead(MAG2_PIN) == LOW);

    // State machine for magnet detection
    switch (magnetState)
    {
    case LEAVING_MAGNET:
      // We are moving off the current magnet. Once both sensors are clear,
      // we can start looking for the next one.
      if (!isMag1Present && !isMag2Present)
      {
        Serial.println("INFO: Cleared current magnet, now seeking next one.");
        magnetState = SEEKING_MAGNET;
        // Ramp up to target speed for seeking
        stepper.setSpeed(targetStepsPerSecond);
      }
      break; // Keep moving

    case SEEKING_MAGNET:
      // We are actively looking for the next magnet (either MAG1 or MAG2). Stop when found.
      if (isMag1Present || isMag2Present)
      {
        stopMotorGently(); // Gentle deceleration
        magnetState = ON_MAGNET;
        if (isMag1Present)
        {
          Serial.println("MAG1 DETECTED! Reached magnet position.");
        }
        else
        {
          Serial.println("MAG2 DETECTED! Reached magnet position.");
        }
        Serial.println("Send 'next' to continue...");
        waitingForCommand = true;
      }
      break;

    case UNKNOWN: // Used for the 'init' command - look for MAG1 only
      if (isMag1Present)
      {
        stopMotorGently(); // Gentle deceleration
        magnetState = ON_MAGNET;
        Serial.println("MAG1 DETECTED! Initialized at home position.");
        Serial.println("Send 'next' to move to next magnet...");
        waitingForCommand = true;
      }
      break;
    }

    // Run the stepper - this handles all acceleration/deceleration automatically
    bool stepperRunning = stepper.run();
    
    // Check if the motor has stopped on a magnet AND the door cycle is armed
    if (!stepperRunning && magnetState == ON_MAGNET && isDoorCycleArmed)
    {
        Serial.println("Motor stopped. Starting automatic door cycle...");
        automaticDoorCycle();
        isDoorCycleArmed = false; // Disarm after cycle runs to prevent re-triggering
    }
    
    // Print progress while moving
    if (stepperRunning && !waitingForCommand)
    {
      printProgress();
    }
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
  else if (command == "init")
  {
    if (!motorConfigured)
    {
      Serial.println("ERROR: Motor not configured! Use 'setup' command first.");
      return;
    }
    handleInitCommand();
  }
  else if (command == "next")
  {
    if (!motorConfigured)
    {
      Serial.println("ERROR: Motor not configured! Use 'setup' command first.");
      return;
    }
    handleNextCommand();
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
    Serial.println("Commands: setup,[RMS_current],[full_current],[pulse/rev],[RPM] | init | next | open | close | stop | rpm [value] | mag | status");
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

void startMotor()
{
  waitingForCommand = false;
  // Ensure maxSpeed is set correctly before starting
  stepper.setMaxSpeed(targetStepsPerSecond);
  // Start at slow speed and ramp up to target
  stepper.setSpeed(startingSpeed);  // Start at ~1 RPM
  stepper.move(1000000);  // Move a very large number of steps (continuous movement)
}

void stopMotor()
{
  stepper.stop();  // AccelStepper handles smooth deceleration
  waitingForCommand = true;
}

void stopMotorGently()
{
  // For magnet detection - move just a few more steps to decelerate smoothly
  long currentPos = stepper.currentPosition();
  stepper.moveTo(currentPos);
  waitingForCommand = true;
}

void printProgress()
{
  // Print progress periodically
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastPrintTime > 2000) // Print every 2 seconds
  {
    float currentSpeed = stepper.speed();
    float currentRPM = (currentSpeed * 60.0) / pulsePerRev;
    Serial.print("Position: ");
    Serial.print(stepper.currentPosition());
    Serial.print(" | Current RPM: ");
    Serial.print(currentRPM, 1);
    Serial.print(" | Target: ");
    Serial.print(targetRPM);
    Serial.print(" | Speed: ");
    Serial.print(currentSpeed, 1);
    Serial.println(" steps/sec");
    lastPrintTime = currentTime;
  }
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
  Serial.println("Door opened.");
}

void closeDoor()
{
  Serial.println("Closing door...");
  moveServoSlow(servo1, 90);  // servo1 to 90 degrees
  moveServoSlow(servo2, 90);  // servo2 to 90 degrees
  Serial.println("Door closed.");
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
      Serial.println("MAG1 DETECTED!");
    }
    else if (mag2Value == LOW)
    {
      Serial.println("MAG2 DETECTED!");
    }
    else
    {
      Serial.println("No magnet");
    }
    delay(300);
  }
  Serial.println("Test complete.");
}

void printStatus()
{
  Serial.println("\n=== SYSTEM STATUS ===");
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

  Serial.print("Current Position: ");
  Serial.println(stepper.currentPosition());

  Serial.print("MAG1 State: ");
  Serial.println(digitalRead(MAG1_PIN) == LOW ? "DETECTED" : "Clear");
  Serial.print("MAG2 State: ");
  Serial.println(digitalRead(MAG2_PIN) == LOW ? "DETECTED" : "Clear");
  
  // Show which magnet is currently detected when on a magnet
  if (magnetState == ON_MAGNET)
  {
    Serial.print("Currently on: ");
    if (digitalRead(MAG1_PIN) == LOW && digitalRead(MAG2_PIN) == LOW)
    {
      Serial.println("Both MAG1 and MAG2");
    }
    else if (digitalRead(MAG1_PIN) == LOW)
    {
      Serial.println("MAG1");
    }
    else if (digitalRead(MAG2_PIN) == LOW)
    {
      Serial.println("MAG2");
    }
    else
    {
      Serial.println("Unknown (no magnet detected)");
    }
  }

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

void handleInitCommand()
{
  isDoorCycleArmed = true; // This movement command permits the door cycle
  // Stop any current movement and reset state
  stopMotor();
  magnetState = UNKNOWN;
  waitingForCommand = false;
  
  Serial.println("INIT: Resetting to find MAG1 home position...");
  
  if (digitalRead(MAG1_PIN) == LOW)
  {
    // Already on MAG1 - confirm position and trigger door cycle
    magnetState = ON_MAGNET;
    Serial.println("MAG1 DETECTED! Already at home position.");
    Serial.println("Send 'next' to move to next magnet...");
    waitingForCommand = true;
    // Trigger automatic door cycle since motor is already stopped
    Serial.println("Motor already stopped. Starting automatic door cycle...");
    automaticDoorCycle();
    isDoorCycleArmed = false; // Disarm the flag immediately to prevent re-triggering
  }
  else
  {
    // Not on MAG1, need to find it
    startMotor();
      Serial.print("Initializing - searching for MAG1 at ");
      Serial.print(targetRPM);
      Serial.print(" RPM (");
      Serial.print(targetStepsPerSecond, 1);
      Serial.println(" steps/sec)");
  }
}

void handleNextCommand()
{
  if (magnetState == ON_MAGNET)
  {
    // Currently on a magnet, need to get off it first
    magnetState = LEAVING_MAGNET;
    if (!stepper.isRunning())
    {
      isDoorCycleArmed = true; // This movement command permits the door cycle
      startMotor();
      Serial.print("Moving off current magnet to find next one at ");
      Serial.print(targetRPM);
      Serial.print(" RPM (");
      Serial.print(targetStepsPerSecond, 1);
      Serial.println(" steps/sec)");
    }
    else
    {
      Serial.println("Motor already running!");
    }
  }
  else if (magnetState == LEAVING_MAGNET || magnetState == SEEKING_MAGNET)
  {
    Serial.println("INFO: Already searching for the next magnet.");
  }
  else // magnetState == UNKNOWN
  {
    // Don't know current state, treat like init
    Serial.println("Magnet state unknown. Use 'init' command first to establish position.");
  }
}
