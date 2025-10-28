# Carousel Controller

This project controls a 12-position carousel using an Arduino, a DM542T stepper driver, and Hall effect sensors for positioning. It allows for precise control over the motor's speed, acceleration, and position via serial commands. The system includes automatic door control and comes pre-configured with default settings for immediate use. After homing to position 1 (MAG1), the carousel uses 840-step intervals between the 12 positions for consistent and reliable movement.

## Hardware Requirements

*   **Microcontroller:** Arduino Uno R4 Minima (or compatible 5V logic board)
*   **Stepper Driver:** DM542T  Stepper Driver
*   **Stepper Motor:** NEMA 23 Stepper Motor (23HS45-4204S or similar bipolar stepper)
*   **Power Supply:** 24V DC Power Supply
*   **Sensors:** 2x Hall Effect Sensors (MAG1 and MAG2 for detecting magnet positions)
*   **Servos:** 2x Servo Motors (for door control)
*   **Jumper Wires**

## Default Configuration

**Important**: Make sure that DM542T driver is enabled to operate with 5V. It has two modes to operate 5V and 24V. Switch must be enabled for 5V.

* **RMS Current:** 3.2A
* **Half Current Mode:** OFF
* **Pulses per Revolution:** 800
* **Target RPM:** 10
* **Homing RPM:** 8
* **Steps Between Positions:** 840 steps (hardcoded for 12 positions)

Send the `home` command to initialize the system and find the home position.

## DM542T Driver Switch Settings

Configure the DM542T driver DIP switches as follows for the default 800 pulses/revolution setting:

| Switch | Position | Function |
| :--- | :--- | :--- |
| **SW1** | OFF | Microstepping Configuration |
| **SW2** | OFF | Microstepping Configuration |
| **SW3** | OFF | Microstepping Configuration |
| **SW4** | OFF | Decay Mode |
| **SW5** | ON  | Current Setting |
| **SW6** | OFF | Current Setting |
| **SW7** | ON  | Current Setting |
| **SW8** | ON  | Current Setting |

*This configuration sets the driver to 800 pulses/revolution with appropriate current settings for the default 3.2A RMS current.*

## Wiring Instructions

### 1. Power Supply to DM542T  Driver

Connect the 24V power supply to the driver's main power terminals.

| Power Supply | DM542T Terminal |
| :--- | :--- |
| **+24V** | VCC |
| **GND**  | GND |

### 2. Stepper Motor to DM542T Driver

Connect the four wires from the bipolar stepper motor to the driver's output terminals.

| Motor Wire | DM542T Terminal |
| :--- | :--- |
| Black (A+) | A+ |
| Green (A-) | A- |
| Red (B+) | B+ |
| Blue (B-) | B- |

### 3. Arduino to DM542T Driver

Connect the control signal pins from the Arduino to the driver.

| Arduino Pin | DM542T Terminal | Function |
| :--- | :--- | :--- |
| **Digital Pin 7** | PUL+ | Step signal |
| **Digital Pin 8** | DIR+ | Direction signal |
| **Digital Pin 4** | ENA+ | Enable/Disable Driver |
| **GND** | PUL- | Step signal ground |
| **GND** | DIR- | Direction signal ground |
| **GND** | ENA- | Enable signal ground |

### 4. Hall Effect Sensors to Arduino

| Sensor | Arduino Pin | Function |
| :--- | :--- | :--- |
| **MAG1 VCC** | 5V | Power |
| **MAG1 GND** | GND | Ground |
| **MAG1 Signal** | **Digital Pin 9** | Position Detection |
| **MAG2 VCC** | 5V | Power |
| **MAG2 GND** | GND | Ground |
| **MAG2 Signal** | **Digital Pin 10** | Position Detection |

### 5. Servo Motors to Arduino

| Servo | Arduino Pin | Function |
| :--- | :--- | :--- |
| **Servo1 Signal** | **Digital Pin 11** | Door Control |
| **Servo1 VCC** | 5V | Power |
| **Servo1 GND** | GND | Ground |
| **Servo2 Signal** | **Digital Pin 12** | Door Control |
| **Servo2 VCC** | 5V | Power |
| **Servo2 GND** | GND | Ground |

## Commands

The system is ready to use immediately with default settings. Here are the available commands:

### `home`
Finds the home position (MAG1) and initializes the system with hardcoded 840-step intervals between all 12 positions. The motor will rotate until MAG1 is detected, then stop and set position 1 as home. This must be run before using position commands.

*   **Example:** `home`

---

### `p1` to `p12`
Moves to a specific position (1-12) using the shortest path. Position 1 (p1) is the home position at MAG1, and all other positions (p2-p12) are spaced 840 steps apart. Upon arrival, the system verifies magnet detection (MAG2) and performs an automatic door cycle.

*   **Format:** `p[number]` (where number is 1-12)
*   **Examples:** `p1`, `p5`, `p12`

---

### `open`
Manually opens the door (only when positioned on a magnet). Both servos will move to open the door mechanism.

*   **Example:** `open`

---

### `close`
Manually closes the door (only when positioned on a magnet). Both servos will return to the closed position.

*   **Example:** `close`

---

### `stop` or `s`
Emergency stop - gently stops the motor with smooth deceleration.

*   **Examples:** `stop` or `s`

---

### `rpm`
Changes the target RPM dynamically. If the motor is running, the speed will change smoothly without stopping. If the motor is stopped, the new RPM will be used for the next movement.

*   **Format:** `rpm [value]` (Range: 1-30 RPM)
*   **Example:** `rpm 15`

---

### `setup` (Optional)
Allows you to override the default configuration if needed. The system works perfectly with defaults, but this command is available for custom configurations.

*   **Format:** `setup,[current],[half_current],[pulse/rev],[RPM]`
*   **Parameters:**
    *   `current`: The RMS current setting for your motor (e.g., `3.2`)
    *   `half_current`: Use `on` or `off` to enable or disable idle current reduction
    *   `pulse/rev`: The microstepping value (must match driver DIP switches)
    *   `RPM`: The target speed in revolutions per minute (1-30)
*   **Example:** `setup,3.2,off,800,10`

---

### `mag`
Tests both Hall effect sensors (MAG1 and MAG2). It will print the sensor status for 10 seconds. Useful for checking if your sensors are working correctly.

*   **Example:** `mag`

---

### `status`
Prints a detailed report of the current system status, including motor configuration, homing status, current position, speed, direction, and sensor states.

*   **Example:** `status`

---

## How It Works

1. **Power on** the system - Arduino initializes with default settings
2. **Run `home`** - System finds MAG1 (home position) and sets up 840-step intervals for all 12 positions
3. **Use `p1`-`p12`** - Navigate to any position using the shortest path (forward or backward)
4. **Automatic door cycle** - Door opens and closes automatically when reaching a position
5. **Manual control** - Use `open`/`close` for manual door operation when stopped at a position

## Position Layout

The carousel has 12 equally-spaced positions:
- **Position 1 (p1):** Home position at MAG1 sensor
- **Positions 2-12 (p2-p12):** Each spaced exactly 840 steps from the previous position
- **Shortest path logic:** System automatically chooses forward or backward movement based on which direction requires fewer steps

## Troubleshooting

- **"System not homed" error:** Run the `home` command first
- **"No magnet detected at target position" error:** Position may have drifted due to missed steps. Run `home` again to re-initialize
- **Motor not moving:** Check that ENABLE_PIN is LOW and driver is powered
- **Inaccurate positioning:** Verify driver DIP switch settings match 800 pulses/revolution