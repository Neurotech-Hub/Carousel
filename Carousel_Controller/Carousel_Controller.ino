#include <Servo.h>

#define STEP_PIN 7
#define DIR_PIN 8
#define ENABLE_PIN 4
#define MAG1_PIN 9
#define MAG2_PIN 10
#define SERVO1_PIN 11
#define SERVO2_PIN 12

// Motor control parameters
int currentDelay = 1000; // Current step delay (microseconds)
int minDelay = 100;      // Target speed delay (microseconds)
int maxDelay = 10000;     // Start speed delay (microseconds)
int accelIncrement = 10; // Acceleration step size
int stepsPerAccel = 50;  // Steps before changing speed

// Motor configuration parameters
float rmsCurrent = 3.0;       // RMS current setting (A)
int pulsePerRev = 800;        // Pulses per revolution from driver setting
bool halfCurrentMode = false; // Half current when stopped
int targetRPM = 60;           // Target RPM speed
bool motorConfigured = false; // Flag to check if setup is complete

// System state variables
bool isMoving = false;
bool waitingForCommand = true;
int stepCount = 0;
unsigned long lastStepTime = 0;

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

void setup()
{
  // Configure motor pins
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  // Configure sensor pin
  pinMode(MAG2_PIN, INPUT_PULLUP);
  pinMode(MAG1_PIN, INPUT_PULLUP);

  // Attach servos and set initial position
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(90); // Start in closed position
  servo2.write(90); // Start in closed position

  // Initialize motor
  digitalWrite(ENABLE_PIN, LOW); // Enable driver
  digitalWrite(DIR_PIN, LOW);    // Set direction (clockwise)
  digitalWrite(STEP_PIN, LOW);

  Serial.begin(115200);
  Serial.println("=== Carosel Controller ===");
  Serial.println("Commands:");
  Serial.println("  'setup,[RMS_current],[half_current],[pulse/rev],[RPM]' - Configure motor");
  Serial.println("    Example: setup,0.71,off,800,60");
  Serial.println("  'init' - Initialize and align with first magnet");
  Serial.println("  'next' - Move to next magnet position");
  Serial.println("  'open' - Open door (only when on magnet)");
  Serial.println("  'close' - Close door (only when on magnet)");
  Serial.println("  'reverse' - Change direction");
  Serial.println("  'rpm [value]' - Set target RPM (1-300, works while running)");
  Serial.println("  'pulse [value]' - Set pulse/rev (200-25600, works while running)");
  Serial.println("  'mag' - Test sensor reading");
  Serial.println("  'status' - Show system status");
  Serial.println();
  Serial.println("Please configure motor first with 'setup' command...");
  Serial.println("Magnet state: UNKNOWN");

  delay(500);
}

void loop()
{
  // Handle serial commands
  handleCommands();

  // Handle motor movement
  if (isMoving && motorConfigured)
  {
    bool isMag1Present = (digitalRead(MAG1_PIN) == LOW);
    bool isMag2Present = (digitalRead(MAG2_PIN) == LOW);

    // State machine for magnet detection
    switch (magnetState)
    {
    case LEAVING_MAGNET:
      // We are moving off the current magnet. Once the sensor is clear,
      // we can start looking for the next one.
      if (!isMag2Present)
      {
        Serial.println("INFO: Cleared current magnet, now seeking next one.");
        magnetState = SEEKING_MAGNET;
      }
      break; // Keep moving

    case SEEKING_MAGNET:
      // We are actively looking for the next magnet (MAG2). Stop when found.
      if (isMag2Present)
      {
        stopMotor();
        magnetState = ON_MAGNET;
        Serial.println("MAGNET DETECTED! Reached next magnet position.");
        Serial.println("Send 'next' to continue...");
        waitingForCommand = true;
      }
      break;

    case UNKNOWN: // Used for the 'init' command - look for MAG1
      if (isMag1Present)
      {
        stopMotor();
        magnetState = ON_MAGNET;
        Serial.println("MAG1 DETECTED! Initialized at home position.");
        Serial.println("Send 'next' to move to next magnet...");
        waitingForCommand = true;
      }
      break;
    }

    // If the motor was not stopped by the state machine, take a step
    if (isMoving)
    {
      if (micros() - lastStepTime >= currentDelay)
      {
        takeStep();

        // Adjust speed towards target
        if (stepCount % stepsPerAccel == 0)
        {
          adjustSpeedToTarget();
        }
      }
    }
  }
}

void handleCommands()
{
  if (Serial.available())
  {
    String command = Serial.readString();
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
        servo1.write(0);
        servo2.write(180);
        Serial.println("Door opened.");
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
        servo1.write(90);
        servo2.write(90);
        Serial.println("Door closed.");
      }
      else
      {
        Serial.println("ERROR: Can only close door when on a magnet.");
      }
    }
    else if (command == "stop")
    {
      stopMotor();
      Serial.println("Motor stopped by user.");
      waitingForCommand = true;
    }
    else if (command == "reverse")
    {
      bool wasMoving = isMoving;
      stopMotor();

      // Toggle direction
      digitalWrite(DIR_PIN, !digitalRead(DIR_PIN));
      Serial.print("Direction changed to: ");
      Serial.println(digitalRead(DIR_PIN) == LOW ? "Clockwise" : "Counter-clockwise");

      if (wasMoving)
      {
        startMotor();
        Serial.println("Motor resumed in new direction...");
      }
    }
    else if (command.startsWith("rpm "))
    {
      if (!motorConfigured)
      {
        Serial.println("ERROR: Configure motor first with 'setup' command.");
        return;
      }
      int newRPM = command.substring(4).toInt();
      if (newRPM >= 1 && newRPM <= 300)
      {
        targetRPM = newRPM;
        calculateOptimalDelays();

        float currentRPM = (1000000.0 / currentDelay) * 60.0 / pulsePerRev;
        Serial.print("Target RPM changed to ");
        Serial.print(targetRPM);
        Serial.print(" (Current: ");
        Serial.print(currentRPM, 1);
        Serial.print(" RPM, Target delay: ");
        Serial.print(minDelay);
        Serial.println("μs)");

        if (isMoving)
        {
          Serial.println("Motor will adjust speed gradually...");
        }
      }
      else
      {
        Serial.println("ERROR: RPM must be between 1-300");
      }
    }
    else if (command.startsWith("pulse "))
    {
      if (!motorConfigured)
      {
        Serial.println("ERROR: Configure motor first with 'setup' command.");
        return;
      }
      int newPulse = command.substring(6).toInt();
      if (newPulse >= 200 && newPulse <= 25600)
      {
        pulsePerRev = newPulse;
        calculateOptimalDelays();

        Serial.print("Pulse/Rev changed to ");
        Serial.print(pulsePerRev);
        Serial.print(" (New target delay: ");
        Serial.print(minDelay);
        Serial.println("μs)");

        if (isMoving)
        {
          Serial.println("Motor will adjust to new microstepping...");
        }
      }
      else
      {
        Serial.println("ERROR: Pulse/Rev must be between 200-25600");
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
      Serial.println("Commands: setup,[RMS_current],[half_current],[pulse/rev],[RPM] | init | next | open | close | stop | reverse | rpm [value] | pulse [value] | mag | status");
    }
  }
}

void parseSetupCommand(String command)
{
  // Parse: setup,current,half_current,pulse_per_rev,rpm
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
      Serial.println("ERROR: Invalid value for half_current. Use 'on' or 'off'.");
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
    if (targetRPM < 1 || targetRPM > 300)
    {
      Serial.println("ERROR: RPM must be 1-300");
      return;
    }

    calculateOptimalDelays();
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
    Serial.print("Calculated Target Delay: ");
    Serial.print(minDelay);
    Serial.println("μs");
    Serial.println("Motor ready! Use 'init' to begin.");
  }
  else
  {
    Serial.println("ERROR: Format = setup,[RMS_current],[half_current(on/off)],[pulse/rev],[RPM]");
    Serial.println("Example: setup,3.0,off,800,60");
  }
}

void calculateOptimalDelays()
{
  // Calculate steps per second for target RPM
  float stepsPerSecond = (float(targetRPM) * float(pulsePerRev)) / 60.0;

  // Calculate base minimum delay
  float baseMinDelay = 1000000.0 / stepsPerSecond; // microseconds per step

  // Calculate final target delay
  minDelay = int(baseMinDelay);

  // Safety limits
  if (minDelay < 100)
    minDelay = 100; // Hardware limit
  if (minDelay > 5000)
    minDelay = 5000; // Reasonable maximum

  // Set starting delay for smooth acceleration
  maxDelay = minDelay * 3; // Start at 3x slower than target
  if (maxDelay > 3000)
    maxDelay = 3000;

  // Adjust acceleration parameters based on speed range
  int speedRange = abs(maxDelay - minDelay);
  accelIncrement = max(5, speedRange / 80);
  stepsPerAccel = max(10, 1500 / targetRPM); // Slower RPM = longer accel
}

// **NEW FUNCTION: Handle both acceleration AND deceleration**
void adjustSpeedToTarget()
{
  if (currentDelay > minDelay)
  {
    // Need to speed up (decrease delay)
    int speedDiff = currentDelay - minDelay;
    int accelStep = max(accelIncrement, speedDiff / 15); // Adaptive acceleration

    currentDelay -= accelStep;
    if (currentDelay < minDelay)
    {
      currentDelay = minDelay;
    }
  }
  else if (currentDelay < minDelay)
  {
    // Need to slow down (increase delay) - THIS WAS MISSING!
    int speedDiff = minDelay - currentDelay;
    int decelStep = max(accelIncrement, speedDiff / 15); // Adaptive deceleration

    currentDelay += decelStep;
    if (currentDelay > minDelay)
    {
      currentDelay = minDelay;
    }
  }
  // If currentDelay == minDelay, we're already at target speed
}

void startMotor()
{
  isMoving = true;
  waitingForCommand = false;

  // Only reset to maxDelay if we're significantly different from target
  if (abs(currentDelay - minDelay) > minDelay / 2)
  {
    currentDelay = maxDelay; // Start slow for big changes
  }
  // Otherwise keep current delay for smooth transitions

  stepCount = 0;
  lastStepTime = micros();
}

void stopMotor()
{
  isMoving = false;
  // Don't reset currentDelay here - preserve speed for restart
  stepCount = 0;
}

void takeStep()
{
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(5); // Minimum pulse width
  digitalWrite(STEP_PIN, LOW);

  stepCount++;
  lastStepTime = micros();

  // Print progress every 100 steps
  if (stepCount % 1000 == 0)
  {
    float currentRPM = (1000000.0 / currentDelay) * 60.0 / pulsePerRev;
    Serial.print("Steps: ");
    Serial.print(stepCount);
    Serial.print(" | Current RPM: ");
    Serial.print(currentRPM, 1);
    Serial.print(" | Target: ");
    Serial.print(targetRPM);
    Serial.print(" | Delay: ");
    Serial.print(currentDelay);
    Serial.println("μs");
  }
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
    Serial.print("Target Delay: ");
    Serial.print(minDelay);
    Serial.println("μs");
  }

  Serial.print("Motor State: ");
  if (isMoving)
  {
    Serial.print("MOVING");
  }
  else
  {
    Serial.print("STOPPED");
  }
  Serial.println();

  if (isMoving)
  {
    float currentRPM = (1000000.0 / currentDelay) * 60.0 / pulsePerRev;
    Serial.print("Current RPM: ");
    Serial.print(currentRPM, 1);
    Serial.print(" | Current Delay: ");
    Serial.print(currentDelay);
    Serial.println("μs");
  }

  Serial.print("Direction: ");
  Serial.println(digitalRead(DIR_PIN) == LOW ? "Clockwise" : "Counter-clockwise");

  Serial.print("Steps Taken: ");
  Serial.println(stepCount);

  Serial.print("MAG1 State: ");
  Serial.println(digitalRead(MAG1_PIN) == LOW ? "DETECTED" : "Clear");
  Serial.print("MAG2 State: ");
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

void handleInitCommand()
{
  if (digitalRead(MAG1_PIN) == LOW)
  {
    // Already on a magnet
    magnetState = ON_MAGNET;
    Serial.println("Already positioned on magnet. State set to ON_MAGNET.");
    Serial.println("Send 'next' to move to next magnet...");
  }
  else
  {
    // Not on a magnet, need to find one. State is already UNKNOWN.
    if (!isMoving)
    {
      startMotor();
      Serial.print("Initializing - searching for first magnet at ");
      Serial.print(targetRPM);
      Serial.print(" RPM ");
      Serial.println(digitalRead(DIR_PIN) == LOW ? "(Clockwise)" : "(Counter-clockwise)");
    }
    else
    {
      Serial.println("Motor already running!");
    }
  }
}

void handleNextCommand()
{
  if (magnetState == ON_MAGNET)
  {
    // Currently on a magnet, need to get off it first
    magnetState = LEAVING_MAGNET;
    if (!isMoving)
    {
      startMotor();
      Serial.print("Moving off current magnet to find next one at ");
      Serial.print(targetRPM);
      Serial.print(" RPM ");
      Serial.println(digitalRead(DIR_PIN) == LOW ? "(Clockwise)" : "(Counter-clockwise)");
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
