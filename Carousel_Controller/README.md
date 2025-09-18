# Carousel Controller

This project controls a stepper motor using an Arduino, a DM542T stepper driver, and a Hall effect sensor for positioning. It allows for precise control over the motor's speed, acceleration, and position via serial commands. It is designed for applications like a product carousel or any system requiring indexed rotational movement.

## Hardware Requirements

*   **Microcontroller:** Arduino Uno R4 Minima (or compatible 5V logic board)
*   **Stepper Driver:** DM542T
*   **Stepper Motor:** NEMA 23 Stepper Motor (23HS45-4204S or similar bipolar stepper)
*   **Power Supply:** 24V DC Power Supply
*   **Sensor:** Hall Effect Sensor (for detecting magnet positions)
*   **Jumper Wires**

## Wiring Instructions

**Important**: Make sure that DM542T driver is enable to operate with 5V. It has two mode to operate 5V and 24V, Switch must be enabled for 5V.

### 1. Power Supply to DM542T Driver

Connect the 24V power supply to the driver's main power terminals.

| Power Supply | DM542T Terminal |
| :--- | :--- |
| **+24V** | VCC / VDD |
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

Connect the control signal pins from the Arduino to the driver. The DM542T can accept 5V logic directly.

| Arduino Pin | DM542T Terminal | Function |
| :--- | :--- | :--- |
| **Digital Pin 7** | PUL+ (Pulse) | Step signal |
| **Digital Pin 8** | DIR+ (Direction) | Direction signal |
| **Digital Pin 4** | ENA+ (Enable) | Enable/Disable Driver |
| **GND** | PUL- | Step signal ground |
| **GND** | DIR- | Direction signal ground |
| **GND** | ENA- | Enable signal ground |

*Note: It's common practice to connect all signal grounds (PUL-, DIR-, ENA-) to the Arduino GND.*

### 4. Hall Effect Sensor to Arduino

| Sensor Pin | Arduino Pin |
| :--- | :--- |
| **VCC / +5V** | 5V |
| **GND** | GND |
| **Signal Out** | **Digital Pin 9** |

## DM542T Driver Configuration

Before running the motor, you must configure the DIP switches on the DM542T driver.

*   **Current Setting:** Set the **Peak Current** to match your motor's specification (e.g., 4.2A for the 23HS45-4204S). The `setup` command's current parameter is for software calculations, but the hardware DIP switch setting is critical.
*   **Microstepping:** Set the **Pulse/Rev** switch to your desired resolution. This value must match the `pulse/rev` parameter you send in the `setup` command. Common values are 800, 1600, 3200, or 6400.
*   **Half Current:** The "Half Current" mode (`SW4`) can be set to ON or OFF. This README documents using the "off" setting and controlling it via the `setup` command. For consistency, keep `SW4` in the `OFF` position and use the software command.

---

### `setup`
Configures the motor parameters. This must be the first command you run. The motor will not start without it.

*   **Format:** `setup,[current],[half_current],[pulse/rev],[RPM]`
*   **Parameters:**
    *   `current`: The RMS current setting for your motor (e.g., `3.0`). This is used for software calculations.
    *   `half_current`: Use `on` or `off` to enable or disable idle current reduction.
    *   `pulse/rev`: The microstepping value set on your driver's DIP switches (e.g., `800`, `6400`).
    *   `RPM`: The target speed in revolutions per minute (e.g., `60`).
*   **Example:** `setup,3.0,off,1600,120`

---

### `start`
Starts the motor movement. The motor will accelerate to the target RPM and continue spinning until a magnet is detected by the Hall effect sensor.

*   **Example:** `start`

---

### `next`
After the motor has stopped at a magnet, this command starts the motor again to find the *next* magnet position.

*   **Example:** `next`

---

### `stop`
Immediately stops the motor.

*   **Example:** `stop`

---

### `reverse`
Changes the direction of motor rotation. If the motor is moving, it will stop, change direction, and then restart.

*   **Example:** `reverse`

---

### `rpm`
Changes the target RPM on-the-fly. The motor will gradually accelerate or decelerate to the new speed.

*   **Format:** `rpm [value]`
*   **Example:** `rpm 150`

---

### `pulse`
Changes the pulse/revolution setting on-the-fly. Make sure this matches your driver's DIP switch settings if you change them while the system is running.

*   **Format:** `pulse [value]`
*   **Example:** `pulse 3200`

---

### `mag`
A utility command to test the Hall effect sensor. It will print the sensor's status ("MAGNET DETECTED!" or "No magnet") for 10 seconds. Useful for checking if your sensor is working correctly.

*   **Example:** `mag`

---

### `status`
Prints a detailed report of the current system status, including whether the motor is configured, its current speed, direction, and sensor state.

*   **Example:** `status`
