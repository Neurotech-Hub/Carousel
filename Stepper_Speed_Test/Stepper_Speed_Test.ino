#include <AccelStepper.h>

// Pin definitions
#define STEP_PIN 7
#define DIR_PIN 8
#define ENABLE_PIN 4

// Motor parameters
float maxAcceleration = 50;      // Steps/second²
float maxSpeed = 200;            // Maximum speed (steps/second)

// AccelStepper object (STEP/DIR interface)
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup()
{
  // Configure motor pins
  pinMode(ENABLE_PIN, OUTPUT);
  
  // Initialize AccelStepper
  digitalWrite(ENABLE_PIN, LOW); // Enable driver
  stepper.setMaxSpeed(maxSpeed);
  stepper.setAcceleration(maxAcceleration);
  stepper.setSpeed(0);           // Start stopped
  
  Serial.begin(115200);
  Serial.println("=== Stepper Speed Test ===");
  Serial.println("Commands:");
  Serial.println("  'speed [value]' - Set speed in steps/sec (-200 to +200)");
  Serial.println("    Examples: speed 10, speed -25, speed 0");
  Serial.println("  'accel [value]' - Set acceleration (1-100 steps/sec²)");
  Serial.println("    Example: accel 25");
  Serial.println("  'maxspeed [value]' - Set max speed (1-500 steps/sec)");
  Serial.println("    Example: maxspeed 150");
  Serial.println("  'stop' - Stop motor gently");
  Serial.println("  'status' - Show current motor status");
  Serial.println("  'pos' - Show current position");
  Serial.println("  'reset' - Reset position to 0");
  Serial.println();
  Serial.println("Motor ready! Try 'speed 10' to start...");
}

void loop()
{
  // Handle serial commands
  handleCommands();
  
  // Run the stepper - this handles all acceleration/deceleration automatically
  stepper.run();
  
  // Print status periodically while moving
  static unsigned long lastStatusTime = 0;
  if (stepper.speed() != 0 && millis() - lastStatusTime > 2000)
  {
    printMovementStatus();
    lastStatusTime = millis();
  }
}

void handleCommands()
{
  if (Serial.available())
  {
    String command = Serial.readString();
    command.trim();
    command.toLowerCase();
    
    if (command.startsWith("speed "))
    {
      float targetSpeed = command.substring(6).toFloat();
      
      // Validate speed range
      if (targetSpeed >= -maxSpeed && targetSpeed <= maxSpeed)
      {
        stepper.setSpeed(targetSpeed);
        Serial.print("Target speed set to: ");
        Serial.print(targetSpeed);
        Serial.println(" steps/sec");
        
        if (targetSpeed != 0)
        {
          // Move continuously in the specified direction
          if (targetSpeed > 0)
          {
            stepper.move(1000000); // Large positive number for continuous movement
          }
          else
          {
            stepper.move(-1000000); // Large negative number for continuous movement
          }
          Serial.println("Motor starting... (will accelerate to target speed)");
        }
        else
        {
          Serial.println("Motor stopping...");
        }
      }
      else
      {
        Serial.print("ERROR: Speed must be between -");
        Serial.print(maxSpeed);
        Serial.print(" and +");
        Serial.println(maxSpeed);
      }
    }
    else if (command.startsWith("accel "))
    {
      float newAccel = command.substring(6).toFloat();
      if (newAccel >= 1 && newAccel <= 100)
      {
        maxAcceleration = newAccel;
        stepper.setAcceleration(maxAcceleration);
        Serial.print("Acceleration set to: ");
        Serial.print(maxAcceleration);
        Serial.println(" steps/sec²");
      }
      else
      {
        Serial.println("ERROR: Acceleration must be between 1-100 steps/sec²");
      }
    }
    else if (command.startsWith("maxspeed "))
    {
      float newMaxSpeed = command.substring(9).toFloat();
      if (newMaxSpeed >= 1 && newMaxSpeed <= 500)
      {
        maxSpeed = newMaxSpeed;
        stepper.setMaxSpeed(maxSpeed);
        Serial.print("Max speed set to: ");
        Serial.print(maxSpeed);
        Serial.println(" steps/sec");
      }
      else
      {
        Serial.println("ERROR: Max speed must be between 1-500 steps/sec");
      }
    }
    else if (command == "stop")
    {
      stepper.stop(); // Gentle deceleration to stop
    }
    else if (command == "status")
    {
      printDetailedStatus();
    }
    else if (command == "pos")
    {
      Serial.print("Current position: ");
      Serial.println(stepper.currentPosition());
    }
    else if (command == "reset")
    {
      stepper.setCurrentPosition(0);
      Serial.println("Position reset to 0");
    }
    else
    {
      Serial.println("Commands: speed [value] | accel [value] | maxspeed [value] | stop | status | pos | reset");
    }
  }
}

void printMovementStatus()
{
  Serial.print("Position: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" | Speed: ");
  Serial.print(stepper.speed(), 1);
  Serial.print(" steps/sec | Target: ");
  Serial.print(stepper.speed(), 1); // Current target speed
  Serial.println(" steps/sec");
}

void printDetailedStatus()
{
  Serial.println("\n=== MOTOR STATUS ===");
  Serial.print("Current Position: ");
  Serial.println(stepper.currentPosition());
  Serial.print("Current Speed: ");
  Serial.print(stepper.speed(), 2);
  Serial.println(" steps/sec");
  Serial.print("Max Speed Setting: ");
  Serial.print(maxSpeed);
  Serial.println(" steps/sec");
  Serial.print("Acceleration Setting: ");
  Serial.print(maxAcceleration);
  Serial.println(" steps/sec²");
  Serial.print("Motor State: ");
  Serial.println(stepper.isRunning() ? "RUNNING" : "STOPPED");
  Serial.print("Target Position: ");
  Serial.println(stepper.targetPosition());
  Serial.print("Distance to Go: ");
  Serial.println(stepper.distanceToGo());
  Serial.println("===================\n");
}
