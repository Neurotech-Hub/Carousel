# Carousel Controller - Complete Project Specification

**Version:** 1.3.0

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [System Architecture](#2-system-architecture)
3. [Hardware Requirements & Configuration](#3-hardware-requirements--configuration)
4. [Hardware Wiring](#4-hardware-wiring)
5. [Software Architecture](#5-software-architecture)
6. [Core Functionality](#6-core-functionality)
7. [State Machines](#7-state-machines)
8. [Command Interface](#8-command-interface)
9. [Implementation Details](#9-implementation-details)
10. [Testing & Validation](#10-testing--validation)

---

## 1. Project Overview

### 1.1 System Purpose
The Carousel Controller is an Arduino-based automated positioning system that manages a 12-position carousel. It provides precise stepper motor control, automated door operations, and beam break monitoring for experimental use. The system is designed for controlled animal testing environments where precise positioning and automated door cycles are critical.

### 1.2 Key Features
- **Precise positioning:** 12-position carousel with magnetic sensor homing
- **Automatic door control:** Dual servo-actuated door mechanism with beam break monitoring
- **Adaptive motion:** Intelligent shortest-path navigation between positions
- **Serial command interface:** Human-readable commands for all operations
- **Robust error handling:** Sensor verification and drift detection
- **Real-time monitoring:** Non-blocking sensor monitoring during operations

### 1.3 System Constraints
- **Position count:** Exactly 12 positions arranged in a circle
- **Stepper driver:** DM542T with STEP/DIR interface
- **Communication:** Serial interface at 115200 baud
- **Position spacing:** 840 steps between adjacent positions (hardcoded)
- **Speed range:** 1-30 RPM
- **Homing speed:** 8 RPM (fixed)

---

## 2. System Architecture

### 2.1 Component Overview

```
┌────────────────────────────────────────────────────────────────┐
│                         ARDUINO UNO                            │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │          Carousel_Controller.ino                         │  │
│  │  - AccelStepper Library (motor control)                  │  │
│  │  - Servo Library (door control)                          │  │
│  │  - Non-blocking serial command processor                 │  │
│  │  - State machines (magnet, beam monitoring)              │  │
│  └──────────────────────────────────────────────────────────┘  │
└──────┬──────────────────┬─────────┬─────────┬──────────┬───────┘
       │                  │         │         │          │
    STEP/DIR        MAG1/MAG2    SERVO     BEAM        SERIAL
       │                │         │         │            │
       ▼                ▼         ▼         ▼            ▼
┌────────────┐   ┌──────────┐  ┌──────┐ ┌────────┐  ┌───────┐
│  DM542T    │   │ Hall     │  │Door  │ │OPB100  │  │PC/USB │
│  Driver    │   │ Sensors  │  │Servos│ │PhotoSen│  │Monitor│
└──────┬─────┘   └──────────┘  └──────┘ └────────┘  └───────┘
       │
       ▼
┌─────────────┐
│  NEMA 23    │
│  Stepper    │
│  Motor      │
└─────────────┘
```

### 2.2 Physical Layout
- **Position 1 (p1):** Home position at MAG1 sensor
- **Positions 2-12 (p2-p12):** Each 840 steps forward from previous position
- **Sensors:** MAG1 (home/position 1), MAG2 (all other positions)
- **Beam breakers:** S1 (mainchamber/inside), S2 (subchamber/outside)

---

## 3. Hardware Requirements & Configuration

### 3.1 Hardware List

| Component | Model/Specification | Quantity | Purpose |
|-----------|-------------------|----------|---------|
| Microcontroller | Arduino Uno R4 Minima (5V logic) | 1 | System control |
| Stepper Driver | DM542T | 1 | Motor control |
| Stepper Motor | NEMA 23 Bipolar (23HS45-4204S or compatible) | 1 | Carousel rotation |
| Power Supply | 24V DC | 1 | Driver power |
| Hall Effect Sensor | Magnetic proximity sensor | 2 | Position detection |
| Servo Motors | Standard hobby servo (5V) | 2 | Door actuation |
| Beam Breaker | OPB100 phototransistor | 2 | Mouse detection |
| Jumper Wires | Various lengths | - | Connections |

### 3.2 DM542T Driver Configuration

**Switch Settings for 800 pulses/revolution:**

| Switch | Position | Function |
|--------|----------|----------|
| SW1 | OFF | Microstepping LSB |
| SW2 | OFF | Microstepping MSB |
| SW3 | OFF | Microstepping GND |
| SW4 | OFF | Decay Mode |
| SW5 | ON | Current LSB |
| SW6 | OFF | Current MSB |
| SW7 | ON | Current Bit 1 |
| SW8 | ON | Current Bit 0 |

**Critical:** DM542T must be switched to 5V mode for Arduino compatibility (internal switch on driver board).

---

## 4. Hardware Wiring

### 4.1 Pin Assignments

```cpp
// Digital Pins
#define STEP_PIN 7           // Step pulse to driver PUL+
#define DIR_PIN 8            // Direction signal to driver DIR+
#define ENABLE_PIN 4         // Enable signal to driver ENA+ (LOW=enabled)
#define MAG1_PIN 9           // Hall sensor for position 1 (home)
#define MAG2_PIN 10          // Hall sensor for positions 2-12
#define SERVO1_PIN 11        // Door servo 1
#define SERVO2_PIN 12        // Door servo 2

// Analog Pins
#define BEAM_S1_PIN A0       // Beam breaker mainchamber side
#define BEAM_S2_PIN A1       // Beam breaker subchamber side
```

### 4.2 Complete Wiring Diagram

#### Arduino to DM542T Driver
```
Arduino Pin 7  → DM542T PUL+  (Step pulse)
Arduino Pin 8  → DM542T DIR+  (Direction)
Arduino Pin 4  → DM542T ENA+  (Enable)
Arduino GND    → DM542T PUL-  (Step ground)
Arduino GND    → DM542T DIR-  (Direction ground)
Arduino GND    → DM542T ENA-  (Enable ground)
```

#### Power to DM542T Driver
```
24V Power Supply + → DM542T VCC
24V Power Supply - → DM542T GND
```

#### Stepper Motor to DM542T Driver
```
Motor Wire (Black A+)  → DM542T A+
Motor Wire (Green A-)  → DM542T A-
Motor Wire (Red B+)    → DM542T B+
Motor Wire (Blue B-)   → DM542T B-
```

#### Hall Effect Sensors
```
MAG1: 5V → VCC, GND → GND, Signal → Pin 9 (INPUT_PULLUP)
MAG2: 5V → VCC, GND → GND, Signal → Pin 10 (INPUT_PULLUP)
```

#### Servo Motors
```
Servo1: Signal → Pin 11, VCC → 5V, GND → GND
Servo2: Signal → Pin 12, VCC → 5V, GND → GND
```

#### Beam Breakers
```
BEAM_S1: Signal → Pin A0 (INPUT_PULLUP)
BEAM_S2: Signal → Pin A1 (INPUT_PULLUP)
```

---

## 5. Software Architecture

### 5.1 Library Dependencies

```cpp
#include <Servo.h>           // Servo control for door mechanism
#include <AccelStepper.h>    // Smooth stepper motor control
```

### 5.2 Global Variables & Constants

```cpp
// Version Information
const String VERSION = "1.3.0";

// Motor Control
float targetStepsPerSecond = 0;    // Calculated target speed
float maxAcceleration = 50;         // steps/second²
int homingRPM = 8;                  // Fixed homing speed

// Motor Configuration
float rmsCurrent = 3.2;             // RMS current (A)
int pulsePerRev = 800;              // Driver microstepping setting
bool halfCurrentMode = false;       // Idle current reduction
int targetRPM = 10;                 // Target operating speed
bool motorConfigured = true;        // Auto-configured flag

// Calibration Data
int magnetIntervals[12];            // Steps between positions [0-11]
int currentPosition = 0;            // 0=uncalibrated, 1-12=calibrated
bool isCalibrated = false;          // Homing complete flag
long currentStepPosition = 0;       // Absolute stepper position

// System State
bool waitingForCommand = true;      // Command waiting flag
bool isDoorCycleArmed = false;      // Door cycle trigger flag

// Beam Breaker Constants
const int BEAM_THRESHOLD = 300;                 // Analog threshold
const int BEAM_DEBOUNCE_READS = 3;              // Confirmation reads
const int BEAM_DEBOUNCE_INTERVAL = 30;          // ms between reads

// Serial Command Buffer
String commandBuffer = "";
const int MAX_COMMAND_LENGTH = 64;
```

### 5.3 Enumerated State Types

```cpp
enum BeamMonitorState {
  BEAM_IDLE,              // Waiting for entry sequence
  BEAM_ENTRY_STARTED,     // S1 broken, waiting for S2
  BEAM_MOUSE_IN_SUBCHAMBER, // Mouse fully entered
  BEAM_EXIT_STARTED       // Mouse exiting, waiting for S1
};

enum MagnetState {
  UNKNOWN,         // Position uncertain
  ON_MAGNET,       // At a magnet position
  LEAVING_MAGNET,  // Moving off magnet
  SEEKING_MAGNET   // Searching for magnet
};
```

### 5.4 Object Instances

```cpp
Servo servo1;                          // Door servo 1
Servo servo2;                          // Door servo 2
AccelStepper stepper(                  // Motor control object
  AccelStepper::DRIVER, 
  STEP_PIN, 
  DIR_PIN
);
```

---

## 6. Core Functionality

### 6.1 System Initialization (setup)

**Sequence:**
1. Configure digital pins (ENABLE_PIN, MAG pins, servo pins, beam pins)
2. Attach servos to pins 11 and 12, set to 90° (closed)
3. Configure AccelStepper:
   - Driver type: DRIVER (STEP/DIR)
   - Max speed: 200 steps/second
   - Acceleration: 50 steps/second²
   - Initial speed: 0
4. Enable driver: Set ENABLE_PIN to LOW
5. Initialize Serial at 115200 baud
6. Print startup banner with version and command list
7. Display default configuration
8. Calculate initial motor speeds
9. 500ms initialization delay

**Critical:** Servos start at 90°, motor is enabled by default.

### 6.2 Main Loop (loop)

**Execution order (non-blocking):**
1. `handleCommands()` - Process serial input
2. `stepper.run()` - Execute motor movements (if configured)
3. `handleBeamMonitoring()` - Monitor beam breaks (if active)

**Frequency:** Executes continuously; stepper.run() must be called frequently for smooth operation.

### 6.3 Motor Control System

#### Speed Calculation
```cpp
void calculateOptimalSpeed()
{
  targetStepsPerSecond = (float(targetRPM) * float(pulsePerRev)) / 60.0;
  
  // Enforce limits
  float minStepsPerSecond = (1.0 * pulsePerRev) / 60.0;   // 1 RPM
  float maxStepsPerSecond = (30.0 * pulsePerRev) / 60.0;  // 30 RPM
  
  if (targetStepsPerSecond < minStepsPerSecond)
    targetStepsPerSecond = minStepsPerSecond;
  if (targetStepsPerSecond > maxStepsPerSecond)
    targetStepsPerSecond = maxStepsPerSecond;
}
```

#### Dynamic Speed Update
```cpp
void updateMotorSpeed()
{
  calculateOptimalSpeed();
  
  if (stepper.isRunning()) {
    stepper.setSpeed(targetStepsPerSecond);  // Dynamic change
  } else {
    stepper.setMaxSpeed(targetStepsPerSecond); // Set for next move
  }
}
```

**Hybrid approach:** Uses `setSpeed()` when running, `setMaxSpeed()` when stopped.

### 6.4 Homing System

**Purpose:** Find MAG1 (home position) and initialize position tracking.

**Sequence:**
1. Save current targetRPM
2. Set homingRPM (8)
3. Calculate homing speed
4. Set max speed for homing
5. Reset position to 0
6. Check if already at MAG1
7. If not, rotate until MAG1 detected (digitalRead(MAG1_PIN) == LOW)
8. Stop gently at MAG1 position
9. Reset stepper position counter to 0
10. Fill magnetIntervals[] array with 840 for all 12 positions
11. Restore original RPM
12. Set flags: isCalibrated=true, currentPosition=1, magnetState=ON_MAGNET

**Critical:** Homing always results in position 1. All intervals are hardcoded to 840 steps.

### 6.5 Position Navigation

**Shortest path algorithm:**
```cpp
int stepsForward = (targetPos - currentPosition + 12) % 12;
int stepsBackward = (currentPosition - targetPos + 12) % 12;

// Handle wrap-around
if (stepsForward == 0) stepsForward = 12;
if (stepsBackward == 0) stepsBackward = 12;

// Choose direction
bool goForward = (stepsForward <= stepsBackward);
int positionsToMove = goForward ? stepsForward : stepsBackward;
```

**Step calculation:**
- Forward: Sum intervals from current to target
- Backward: Subtract intervals from current to target

**Verification:** After arrival, check MAG2 sensor. If not detected, system is marked uncalibrated.

### 6.6 Door Control

**Servo positions:**
- Closed: servo1=90°, servo2=90°
- Open: servo1=0°, servo2=180°

**Movement:** Servo moves in 10° increments with 20ms delays for smooth motion.

**Sequences:**
- Manual open: `openDoor()` → Move servos → Done
- Manual close: `closeDoor()` → Stop beam monitoring → Move servos → Done
- Automatic cycle: `automaticDoorCycle()` → Open → Start beam monitoring

### 6.7 Beam Break Monitoring

**Purpose:** Detect mouse entry/exit through door using two-beam sequence.

**Sensor configuration:**
- S1 (A0): Mainchamber side (inside)
- S2 (A1): Subchamber side (outside)
- Threshold: 300 (analogRead)
- Debounce: 3 consecutive reads with 30ms intervals

**Detection sequence:**
1. **Entry:** S1 breaks → S2 breaks → "Mouse in subchamber"
2. **Exit:** S2 breaks → S1 breaks → Close door → Stop monitoring

**State machine:**
```
BEAM_IDLE 
  → (S1 breaks) 
BEAM_ENTRY_STARTED 
  → (S2 breaks) 
BEAM_MOUSE_IN_SUBCHAMBER 
  → (S2 breaks) 
BEAM_EXIT_STARTED 
  → (S1 breaks) 
CLOSE DOOR → BEAM_IDLE
```

**Important:** State machine is non-blocking and monitors continuously when active.

---

## 7. State Machines

### 7.1 Magnet State Machine

```cpp
UNKNOWN 
  → Homing or drift detection
ON_MAGNET 
  → Door operations allowed
LEAVING_MAGNET 
  → Transitional state
SEEKING_MAGNET 
  → Moving to target
```

**State transitions:**
- Homing completes → ON_MAGNET (position 1)
- Position command starts → SEEKING_MAGNET or LEAVING_MAGNET
- Magnet detected at target → ON_MAGNET
- No magnet at target → UNKNOWN, marked uncalibrated

### 7.2 Beam Monitoring State Machine

```cpp
BEAM_IDLE 
  → Waiting for S1 to break
  → (S1 breaks) → BEAM_ENTRY_STARTED

BEAM_ENTRY_STARTED 
  → Waiting for S2 to break
  → (S2 breaks) → BEAM_MOUSE_IN_SUBCHAMBER
  → (S1 clears without S2) → ignore (backed out)

BEAM_MOUSE_IN_SUBCHAMBER 
  → Waiting for S2 to break
  → (S2 breaks) → BEAM_EXIT_STARTED

BEAM_EXIT_STARTED 
  → Waiting for S1 to break
  → (S1 breaks) → CLOSE DOOR, stop monitoring
  → (S2 clears without S1) → ignore (backed out)
```

**Key behavior:** Silent transitions except "Mouse in subchamber" and "Mouse returned - closing door".

---

## 8. Command Interface

### 8.1 Serial Communication

**Configuration:**
- Baud rate: 115200
- Buffer size: 64 characters
- Non-blocking: Read one character per loop iteration

### 8.2 Command List

#### `home`
- **Purpose:** Find home position and initialize system
- **Validation:** Requires motorConfigured=true
- **Action:** Homing sequence, set currentPosition=1, isCalibrated=true
- **Output:** "HOMING COMPLETE" with intervals info

#### `p1` through `p12`
- **Purpose:** Navigate to specific position
- **Validation:** 
  - Requires isCalibrated=true
  - Current position must not be unknown (0)
  - Cannot execute while beam monitoring active
- **Action:** 
  - Calculate shortest path
  - Execute movement
  - Verify magnet at arrival
  - Trigger automatic door cycle
- **Output:** Movement info, arrival confirmation

#### `open`
- **Purpose:** Manually open door
- **Validation:** Requires magnetState=ON_MAGNET
- **Action:** Move servos to open positions

#### `close`
- **Purpose:** Manually close door
- **Validation:** Requires magnetState=ON_MAGNET
- **Action:** Stop beam monitoring, move servos to closed positions

#### `stop` or `s`
- **Purpose:** Emergency stop with smooth deceleration
- **Action:** 
  - Disarm door cycle
  - Call stepper.stop()
  - Set waitingForCommand=true

#### `rpm [value]`
- **Purpose:** Change target RPM dynamically
- **Parameters:** 1-30 (integer)
- **Validation:** Requires motorConfigured=true
- **Action:** Update targetRPM, call updateMotorSpeed()
- **Example:** `rpm 15`

#### `mag`
- **Purpose:** Test Hall effect sensors
- **Action:** Print sensor readings for 10 seconds
- **Output:** "Home sensor detected" or "Position sensor detected" or "No sensor detected"

#### `beam`
- **Purpose:** Test beam breaker sensors
- **Action:** Print analog values and blocked/clear status for 10 seconds
- **Output:** "S1=XXX BLOCKED/CLEAR | S2=XXX BLOCKED/CLEAR"

#### `status`
- **Purpose:** Display complete system state
- **Output:** 
  - Version
  - Motor configuration
  - Homing status
  - Current position
  - Motor state and speed
  - Direction
  - Stepper position
  - Sensor states
  - Magnet state
  - Beam monitoring state and status
  - Current beam readings

#### `setup,[current],[half_current],[pulse/rev],[RPM]`
- **Purpose:** Configure motor parameters (optional, defaults provided)
- **Parameters:**
  - current: 0.5-4.2A (float)
  - half_current: "on" or "off" (case insensitive)
  - pulse/rev: 200-25600 (integer)
  - RPM: 1-30 (integer)
- **Example:** `setup,3.2,off,800,10`
- **Validation:** Range checks, rejects invalid inputs
- **Action:** Updates globals, calculates speeds, sets motorConfigured=true

### 8.3 Command Processing

**Buffer management:**
- Accumulate characters until '\n' or '\r'
- Max length: 64 characters (overflow protection)
- Clear buffer after processing

**Input normalization:**
- Trim whitespace
- Convert to lowercase

**Error handling:**
- Invalid commands → Print command list
- Missing prerequisites → Print error message
- Invalid parameters → Print parameter range error

---

## 9. Implementation Details

### 9.1 Non-Blocking Serial Reading

```cpp
void handleCommands()
{
  while (Serial.available() > 0) {
    char incomingChar = Serial.read();
    
    if (incomingChar == '\n' || incomingChar == '\r') {
      if (commandBuffer.length() > 0) {
        processCommand(commandBuffer);
        commandBuffer = "";
      }
    } else {
      if (commandBuffer.length() < MAX_COMMAND_LENGTH) {
        commandBuffer += incomingChar;
      } else {
        commandBuffer = "";
        Serial.println("ERROR: Command too long!");
      }
    }
  }
}
```

**Key:** One character per call, no blocking delays for soft stop command.

### 9.2 Debounced Sensor Reading

```cpp
bool isBeamBroken(int pin)
{
  int confirmedReads = 0;
  
  for (int i = 0; i < BEAM_DEBOUNCE_READS; i++) {
    int value = analogRead(pin);
    if (value > BEAM_THRESHOLD) {
      confirmedReads++;
    } else {
      return false;  // Any read below threshold = not broken
    }
    
    if (i < BEAM_DEBOUNCE_READS - 1) {
      delay(BEAM_DEBOUNCE_INTERVAL);
    }
  }
  
  return (confirmedReads == BEAM_DEBOUNCE_READS);
}
```

**Requirement:** `n` consecutive reads above `threshold` = beam broken.

### 9.3 Smooth Servo Movement

```cpp
void moveServoSlow(Servo &servo, int targetPosition)
{
  int currentPosition = servo.read();
  int direction = (targetPosition > currentPosition) ? 1 : -1;
  
  for (int pos = currentPosition; pos != targetPosition; pos += direction * 10) {
    if ((direction == 1 && pos > targetPosition) || 
        (direction == -1 && pos < targetPosition)) {
      pos = targetPosition;
    }
    servo.write(pos);
    delay(20);
    if (pos == targetPosition) break;
  }
}
```

**Movement:** 10° increments, 20ms delays = ~180ms for 90° movement.

### 9.4 Position Tracking

**Absolute positioning:**
- `currentStepPosition`: Long integer tracking absolute stepper steps
- Updated only on complete movements
- Never reset except during homing

**Relative positioning:**
- `currentPosition`: 0-12 indicating logical position
- 0 = uncalibrated
- 1-12 = valid positions

**Verification:**
- After each move, check MAG2 sensor
- If no sensor detected → mark uncalibrated
- User must re-home

### 9.5 Door Cycle Triggering

**Arming mechanism:**
- Set `isDoorCycleArmed=true` before position movement
- Check flag after arrival at magnet
- If armed and on magnet → trigger automatic door cycle
- Disarm after execution

**Disabled cases:**
- Manual stop command
- Beam monitoring already active
- Not at a magnet

### 9.6 Error Recovery

**Drift detection:**
- If no magnet detected after position command → uncalibrated
- User must run `home` to restore

**Beam monitoring conflict:**
- Position commands rejected while beam monitoring active
- User must close door first

**State inconsistencies:**
- Manual stop clears door cycle arming
- Position unknown resets to 0, requires homing

---

## 10. Testing & Validation

### 10.1 Startup Sequence

**Expected output:**
```
=== Carousel Controller 1.3.0 ===
Commands:
  'home' - Find home position (MAG1) and initialize system
  'p1' to 'p12' - Move to specific position (shortest path)
  'open' - Open door (only when on magnet)
  'close' - Close door (only when on magnet)
  'stop' or 's' - Emergency stop
  'rpm [value]' - Set target RPM (1-30)
  'mag' - Test magnetic sensor reading
  'beam' - Test beam breaker sensors
  'status' - Show system status
  'setup,[RMS_current],[full_current],[pulse/rev],[RPM]' - Configure motor (optional)
    Example: setup,3.2,off,800,10

=== DEFAULT CONFIGURATION ===
RMS Current: 3.2A
Half Current Mode: OFF
Pulses/Revolution: 800
Target RPM: 10

⚠️  IMPORTANT: Run 'home' command first to initialize the system!
Status: NOT HOMED
```

### 10.2 Homing Validation

**Test:** Send `home` command

**Expected:**
1. Motor rotates until MAG1 detected
2. Stops at MAG1
3. Output: "HOMING COMPLETE"
4. Status shows: isCalibrated=true, currentPosition=1
5. `status` command shows "HOMED ✓"

### 10.3 Navigation Validation

**Test sequence:**
1. `home` → Verify position 1
2. `p6` → Verify forward movement
3. `p2` → Verify backward (shortest path)
4. `p12` → Verify wrap-around navigation
5. `status` → Verify all position tracking

**Critical:** Each move should verify magnet detection on arrival.

### 10.4 Door Cycle Validation

**Test sequence:**
1. `p3` → Wait for automatic door open
2. Verify beam monitoring starts
3. Simulate mouse entry (S1 → S2)
4. Verify "Mouse in subchamber" message
5. Simulate exit (S2 → S1)
6. Verify door closes

**Manual test:**
1. `p1` → Arrive at position
2. `open` → Verify door opens
3. `close` → Verify door closes and beam monitoring stops

### 10.5 Speed Control Validation

**Test sequence:**
1. `rpm 5` → Verify slow movement
2. `p12` → Measure travel time
3. `rpm 20` → Verify faster movement
4. `p1` → Measure travel time
5. `rpm 10` → Restore default

**Expected:** Dynamic speed changes without stopping.

### 10.6 Error Handling Validation

**Test cases:**
1. `p1` before homing → "System not homed" error
2. `open` when not on magnet → "Can only open door when on a magnet" error
3. `p5` while beam monitoring → "Cannot move while beam monitoring is active" error
4. `stop` during movement → Smooth deceleration

### 10.7 Sensor Validation

**Test `mag` command:**
- Bring MAG1 near sensor → "Home sensor detected"
- Bring MAG2 near sensor → "Position sensor detected"
- No sensor → "No sensor detected"

**Test `beam` command:**
- Block S1 → Show BLOCKED
- Clear S1 → Show CLEAR
- Analog values printed

### 10.8 Complete Status Validation

**Test `status` command output includes:**
- ✓ Version number
- ✓ Motor configuration (all parameters)
- ✓ Homing status with position
- ✓ Motor state (MOVING/STOPPED)
- ✓ Current RPM and speed
- ✓ Direction
- ✓ Stepper position
- ✓ Sensor states
- ✓ Magnet state
- ✓ Beam monitoring state
- ✓ Beam sensor readings

---

## 11. Additional Implementation Notes

### 11.1 Critical Timing

- **stepper.run():** Must be called frequently (every loop iteration)
- **Servo delays:** 20ms per 10° increment (non-negotiable)
- **Beam debounce:** 30ms intervals for 3 reads = ~90ms per check
- **Serial buffer:** Processes one character at a time

### 11.2 Memory Considerations

- **String usage:** Minimized (commandBuffer only)
- **Fixed arrays:** magnetIntervals[12] (48 bytes)
- **No dynamic allocation:** All memory statically allocated

### 11.3 Library Versions

- **AccelStepper:** Latest stable (tested with 1.64)
- **Servo:** Arduino standard library
- **Hardware Serial:** Native Arduino

### 11.4 Hardware Assumptions

- **Stepper motor:** Bipolar, 4-wire
- **Hall sensors:** Active LOW when magnet present
- **Servos:** Standard 90° range (±45° from center)
- **Beam breakers:** OPB100 phototransistors, analog output
- **Arduino:** 5V logic levels

### 11.5 Future Extensibility Points

- **Position intervals:** Currently hardcoded, could be calibrated
- **Door cycle logic:** Fixed sequence, could be configurable
- **Speed profiles:** Single acceleration value, could be position-dependent
- **Beam thresholds:** Fixed constants, could be adaptive

---

## 12. Compilation & Deployment

### 12.1 Required Libraries

```bash
# AccelStepper (install via Arduino Library Manager)
# Search: "AccelStepper by Mike McCauley"
```

### 12.2 Build Configuration

- **Board:** Arduino Uno (or compatible)
- **Processor:** ATmega328P or similar
- **Programmer:** AVRISP mkII or compatible
- **COM Port:** Assign appropriate serial port

### 12.3 Upload Process

1. Connect Arduino via USB
2. Select board and port in IDE
3. Verify code compiles without errors
4. Upload to board
5. Open Serial Monitor at 115200 baud
6. Wait for startup banner
7. Run `home` command

### 12.4 Initial Calibration

**First-time setup:**
1. Power on system
2. Connect to Serial Monitor
3. Send `home` command
4. Wait for completion
5. Send `status` to verify
6. Ready for operations

---

## 13. Troubleshooting Guide

### 13.1 Motor Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| Motor not moving | Driver not enabled | Check ENABLE_PIN is LOW |
| Jerky movement | Acceleration too high | Verify maxAcceleration=50 |
| Wrong speed | Pulse/rev mismatch | Check driver DIP switches |
| Missed steps | Current too low | Increase rmsCurrent |
| Excessive heat | Current too high | Decrease rmsCurrent |

### 13.2 Sensor Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| Never homes | MAG1 not detected | Check wiring, test with `mag` command |
| Misses positions | MAG2 not detected | Verify sensor placement |
| False triggers | Noise in sensor | Check pullup resistors enabled |
| Beam not detected | Threshold too high | Lower BEAM_THRESHOLD |

### 13.3 Door Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| Door doesn't open | Not on magnet | Verify magnetState=ON_MAGNET |
| Door opens/closes wrong | Servo wiring error | Check pin assignments |
| Jittery movement | Servo delay too short | Verify 20ms delay |
| Door stuck | Mechanical obstruction | Check physical mechanism |

### 13.4 Communication Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| No serial output | Wrong baud rate | Set 115200 |
| Commands ignored | Buffer overflow | Check MAX_COMMAND_LENGTH |
| Partial commands | Serial timing | Verify non-blocking reads |

---

## 15. Summary

This specification provides complete information to replicate the Carousel Controller version 1.3.0. The system combines precise stepper motor control, automated door operations, and intelligent sensor monitoring to create a robust 12-position carousel system. Key innovations include non-blocking operation, shortest-path navigation, adaptive speed control, and comprehensive error handling.

The default configuration is production-ready, requiring only the `home` command to initialize. All features are accessible via a human-readable serial command interface, making the system both powerful and user-friendly.

