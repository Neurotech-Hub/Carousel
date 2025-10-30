#include <Servo.h>

Servo servo1;          // Create servo object for motor 1
Servo servo2;          // Create servo object for motor 2
int servo1Pin = 2;     // Servo 1 connected to pin 2
int servo2Pin = 3;     // Servo 2 connected to pin 3

void setup() {
  Serial.begin(9600);           // Initialize serial communication
  servo1.attach(servo1Pin);     // Attach servo 1 to pin 2
  servo2.attach(servo2Pin);     // Attach servo 2 to pin 3
  
  // Set initial positions
  servo1.write(90);
  servo2.write(90);
  
  Serial.println("Dual Servo Controller Ready!");
  Serial.println("Commands:");
  Serial.println("1,angle - Control Servo 1 (Pin 2)");
  Serial.println("2,angle - Control Servo 2 (Pin 3)");
  Serial.println("Example: 1,90 or 2,100");
}

void loop() {
  // Check if serial data is available
  if (Serial.available() > 0) {
    // Read the incoming string
    String input = Serial.readStringUntil('\n');
    input.trim(); // Remove whitespace
    
    // Parse the comma-separated command
    int commaIndex = input.indexOf(',');
    
    if (commaIndex > 0) {
      // Extract servo number and angle
      String servoNumStr = input.substring(0, commaIndex);
      String angleStr = input.substring(commaIndex + 1);
      
      int servoNum = servoNumStr.toInt();
      int angle = angleStr.toInt();
      
      // Validate angle range
      if (angle >= 0 && angle <= 180) {
        // Route command to appropriate servo
        if (servoNum == 1) {
          servo1.write(angle);
          Serial.print("Servo 1 (Pin 2) moved to: ");
          Serial.print(angle);
          Serial.println(" degrees");
        }
        else if (servoNum == 2) {
          servo2.write(angle);
          Serial.print("Servo 2 (Pin 3) moved to: ");
          Serial.print(angle);
          Serial.println(" degrees");
        }
        else {
          Serial.println("Error: Invalid servo number. Use 1 or 2");
        }
      } else {
        Serial.println("Error: Angle must be between 0-180 degrees");
      }
    } else {
      Serial.println("Error: Invalid format. Use: servoNumber,angle");
      Serial.println("Example: 1,90 or 2,100");
    }
  }
}
