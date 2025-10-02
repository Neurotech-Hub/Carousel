# Carousel Controller

This project controls a stepper motor using an Arduino, a DM542T stepper driver, and Hall effect sensors for positioning. It allows for precise control over the motor's speed, acceleration, and position via serial commands. The system includes automatic door control and comes pre-configured with default settings for immediate use. It is designed for applications like a product carousel or any system requiring indexed rotational movement.

## Hardware Requirements

*   **Microcontroller:** Arduino Uno R4 Minima (or compatible 5V logic board)
*   **Stepper Driver:** DM542T  Stepper Driver
*   **Stepper Motor:** NEMA 23 Stepper Motor (23HS45-4204S or similar bipolar stepper)
*   **Power Supply:** 24V DC Power Supply
*   **Sensors:** 2x Hall Effect Sensors (MAG1 and MAG2 for detecting magnet positions)
*   **Servos:** 2x Servo Motors (for door control)
*   **Jumper Wires**

## Default Configuration

**Important**: Make sure that DM542T driver is enable to operate with 5V. It has two mode to operate 5V and 24V, Switch must be enabled for 5V.

* **RMS Current:** 3.2A
* **Half Current Mode:** OFF
* **Pulses per Revolution:** 800
* **Target RPM:** 10

Send the `init` command to begin operation.

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

### `init`
Initializes the carousel and finds the home position (MAG1). The motor will start moving and automatically stop when MAG1 is detected. An automatic door cycle will then be performed.

*   **Example:** `init`

---

### `next`
After the motor has stopped at a magnet position, this command starts the motor again to find the *next* magnet position. The motor will automatically stop when MAG1 or MAG2 is detected and perform a door cycle.

*   **Example:** `next`

---

### `open`
Manually opens the door (only when positioned on a magnet). Both servos will move to open the door mechanism.

*   **Example:** `open`

---

### `close`
Manually closes the door (only when positioned on a magnet). Both servos will return to the closed position.

*   **Example:** `close`

---

### `stop`
Gently stops the motor with smooth deceleration.

*   **Example:** `stop`

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
Prints a detailed report of the current system status, including motor configuration, current speed, position, direction, and sensor states.

*   **Example:** `status`