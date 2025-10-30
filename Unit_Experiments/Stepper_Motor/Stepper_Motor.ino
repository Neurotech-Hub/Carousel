#define STEP_PIN 7
#define DIR_PIN 8
#define ENABLE_PIN 4

// Acceleration parameters
int currentDelay = 3000;    // Start very slow (microseconds)
int minDelay = 100;         // Maximum speed (microseconds)
int accelIncrement = 5;     // Acceleration step size
int stepsPerAccel = 20;     // Steps before changing speed

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  
  // Enable the driver (LOW = enabled)
  digitalWrite(ENABLE_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  digitalWrite(STEP_PIN, LOW);
  
  Serial.begin(115200);
  Serial.println("DM542T Stepper Controller Ready");
  Serial.println("Commands: +[steps] for CW, -[steps] for CCW");
  Serial.println("Example: +800 (one full rotation at 800 steps/rev)");
  
  delay(500); // Give driver time to initialize
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    
    if (command.startsWith("+")) {
      int steps = command.substring(1).toInt();
      if (steps > 0) {
        Serial.print("Moving ");
        Serial.print(steps);
        Serial.println(" steps clockwise");
        
        digitalWrite(DIR_PIN, LOW);  // Clockwise
        moveStepsWithAccel(steps);
      }
      
    } else if (command.startsWith("-")) {
      int steps = command.substring(1).toInt();
      if (steps > 0) {
        Serial.print("Moving ");
        Serial.print(steps);
        Serial.println(" steps counterclockwise");
        
        digitalWrite(DIR_PIN, HIGH); // Counterclockwise
        moveStepsWithAccel(steps);
      }
      
    } else if (command.equals("test")) {
      Serial.println("Running test sequence...");
      testSequence();
    }
  }
}

void moveStepsWithAccel(int totalSteps) {
  currentDelay = 3000; // Reset to slow speed
  int stepCount = 0;
  
  while (stepCount < totalSteps) {
    // Generate step pulse
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);  // Minimum pulse width
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(currentDelay);
    
    stepCount++;
    
    if (stepCount % stepsPerAccel == 0) {
      if (currentDelay > minDelay) {
        // Aggressive acceleration at start, gentle near max speed
        if (currentDelay > 1000) {
          currentDelay -= accelIncrement * 4;
        } else if (currentDelay > 300) {
          currentDelay -= accelIncrement * 2;
        } else {
          currentDelay -= accelIncrement;
        }
        
        if (currentDelay < minDelay) {
          currentDelay = minDelay;
        }
      }
    }
    
    // Deceleration for last 20% of steps
    int decelStart = totalSteps * 0.8;
    if (stepCount > decelStart) {
      currentDelay += accelIncrement;
      if (currentDelay > 2000) currentDelay = 2000;
    }
  }
  
  Serial.println("Movement complete");
  delay(100);
}

void testSequence() {
  Serial.println("Test 1: 200 steps CW (one full rotation at 1:1)");
  digitalWrite(DIR_PIN, LOW);
  moveStepsWithAccel(200);
  delay(1000);
  
  Serial.println("Test 2: 200 steps CCW");
  digitalWrite(DIR_PIN, HIGH);
  moveStepsWithAccel(200);
  delay(1000);
  
  Serial.println("Test 3: 800 steps CW (one full rotation at 4:1 microstep)");
  digitalWrite(DIR_PIN, LOW);
  moveStepsWithAccel(800);
  delay(1000);
  
  Serial.println("Test sequence complete");
}
