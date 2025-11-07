# Carousel Controller v1.4.0 - Dwell Time Recording System

Complete Arduino-based 12-position carousel controller with automatic door operation and dwell time data logging for behavioral experiments.

## 🎯 New in v1.4.0

- ✨ **Dwell Time Tracking**: Automatically records time spent in subchamber
- 📊 **Excel Data Logging**: Date-based Excel files (Carousel_MMDDYY.xlsx)
- 🖥️ **Python GUI**: Complete tkinter-based control interface
- 📡 **Real-time Status**: Live display of magnet state and mouse status
- 🔄 **Session Management**: Trial counter resets on each home command
- 📈 **Data Export**: CSV format with timestamps and event types (AUTO/MANUAL)

## 📋 Project Overview

### System Components

```
┌─────────────────────────────────────────────────────────┐
│  Arduino UNO R4 Minima                                  │
│  Carousel_Controller.ino v1.4.0                         │
│  ├─ Stepper Motor Control (DM542T Driver)              │
│  ├─ Dual Servo Door Mechanism                          │
│  ├─ Hall Effect Sensor Homing (12 positions)           │
│  ├─ Dual Beam Breaker Detection                        │
│  └─ Dwell Time Tracking & Serial Logging               │
└─────────────────────────────────────────────────────────┘
                          ↕ USB Serial (115200)
┌─────────────────────────────────────────────────────────┐
│  Python GUI Application                                 │
│  ├─ Serial Connection Management                        │
│  ├─ Real-time Status Display                           │
│  ├─ Position & Door Control                            │
│  ├─ Excel Data Logging                                 │
│  └─ Communication Log                                   │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│  Data Storage (./data/)                                 │
│  Carousel_110725.xlsx                                   │
│  Trial | Position | Entry | Exit | Dwell | Event | PC_Time
│  1     | 5        | 12543 | 18865| 6.32  | AUTO  | 10:15:23
│  2     | 8        | 45230 | 50100| 4.87  | MANUAL| 10:18:45
└─────────────────────────────────────────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

**Hardware:**
- Arduino UNO R4 Minima (5V logic)
- DM542T Stepper Driver
- NEMA 23 Stepper Motor
- 2x Hobby Servos (5V)
- 2x Hall Effect Sensors
- 2x OPB100 Beam Breakers

**Software:**
- Arduino IDE 2.0+
- Python 3.7+
- Libraries: `AccelStepper`, `Servo` (Arduino), `pyserial`, `openpyxl`, `pandas` (Python)

### Installation

#### 1. Arduino Firmware

```bash
# Open Arduino IDE
# File → Open → Carousel_Controller/Carousel_Controller.ino
# Tools → Board → Arduino UNO R4 Minima
# Tools → Port → Select COM port
# Upload to Arduino
```

**Required Arduino Libraries:**
- `AccelStepper` by Mike McCauley
- `Servo` (built-in)

#### 2. Python GUI

```bash
cd Python_GUI
pip install -r requirements.txt
python carousel_gui.py
```

### First Run

1. **Connect Arduino** via USB
2. **Open GUI** and select COM port (or use auto-detect)
3. **Click "Connect"** (wait for green indicator)
4. **Click "Home"** to initialize system
5. **Select position** (p1-p12) and click "Go"
6. **Watch automation**: Door opens → Mouse enters/exits → Door closes → Data logged
7. **View data**: Click "Open Folder" to see Excel file

## 📊 Data Collection

### Excel File Format

Files are automatically created with format: `Carousel_MMDDYY.xlsx`

| Column | Type | Description |
|--------|------|-------------|
| Trial | int | Sequential trial number (resets on home) |
| Position | int | Position 1-12 where trial occurred |
| EntryTime | int | Arduino millis() when mouse entered subchamber |
| ExitTime | int | Arduino millis() when mouse exited subchamber |
| DwellTime | float | Time in subchamber (seconds, 2 decimals) |
| Event | str | 'AUTO' (position command) or 'MANUAL' (manual open) |
| Timestamp | str | PC timestamp when data was saved |

### Example Data

```
Trial,Position,EntryTime,ExitTime,DwellTime,Event,Timestamp
1,5,12543,18865,6.32,AUTO,2025-11-07 10:15:23
2,8,45230,50100,4.87,MANUAL,2025-11-07 10:18:45
3,3,65100,71250,6.15,AUTO,2025-11-07 10:22:10
```

### Data Storage

- **Location**: `./data/` folder in Python_GUI directory
- **Naming**: `Carousel_MMDDYY.xlsx` (e.g., `Carousel_110725.xlsx`)
- **Behavior**: Appends to existing file if same date, creates new file for new date
- **Backup**: Recommend daily backup of data folder

## 🎮 Usage Guide

### GUI Sections

#### 1. Serial Connection
- **Port Dropdown**: Select Arduino COM port
- **Auto-detect**: Automatically find Arduino
- **Refresh**: Manually refresh port list
- **Connect/Disconnect**: Toggle connection
- **Status Indicator**: Green (connected) / Red (disconnected)

#### 2. System Status
- **Magnet State**: `ON_MAGNET` (green) / `UNKNOWN` (gray)
- **Mouse Status**: `IDLE` (blue) / `ENTRY` (orange) / `ENTERED` (green) / `EXIT` (orange)
- **Position**: Current position (p1-p12)
- **Device Status**: Request full status report
- **Emergency Stop**: Immediate motor stop

#### 3. Controls
- **Home**: Initialize system, reset trial counter, find home position
- **Position (p1-p12)**: Move to selected position (triggers AUTO door cycle)
- **Manual Door**:
  - **Open**: Manually open door (triggers MANUAL recording)
  - **Close**: Manually close door and save data
- **Troubleshooting**:
  - **Test Mag**: Test magnetic sensors for 10 seconds
  - **Test Beam**: Test beam breakers for 10 seconds

#### 4. Data Storage
- **Current File**: Shows today's Excel filename
- **Trials Today**: Count of trials in current file
- **Location**: Full path to data folder
- **Open Folder**: Opens data folder in file explorer

#### 5. Communication Log
- **Color Coding**:
  - **Black**: General information
  - **Purple**: Commands sent to Arduino
  - **Blue**: Data packets (logged entries)
  - **Green**: Status updates
  - **Orange**: Warnings
  - **Red**: Errors
- **Clear Log**: Erase current log
- **Save Log**: Export log to text file

### Typical Workflow

#### Automatic Experiment (Recommended)

```
1. Connect Arduino via GUI
2. Click "Home" (resets trial counter to 0)
3. Select "p5" from dropdown
4. Click "Go"
   → Arduino moves to position 5
   → Door opens automatically (EVENT: AUTO)
   → Beam monitoring starts
   → Wait for mouse to enter (S1 → S2)
   → "Mouse in subchamber" message appears
   → Wait for mouse to exit (S2 → S1)
   → Door closes automatically
   → Data packet sent: DATA,1,5,12543,18865,6.32,AUTO
   → Excel file updated
5. Repeat steps 3-4 for additional trials
```

#### Manual Control

```
1. Move to position: Select position, click "Go"
2. Manually open door: Click "Open" button (EVENT: MANUAL)
3. Wait for mouse activity
4. Manually close door: Click "Close" button
   → Data saved with MANUAL event type
```

## 📡 Serial Protocol

### Arduino → PC Messages

#### Data Packets
```
DATA,Trial,Position,EntryTime,ExitTime,DwellTime,Event
DATA,1,5,12543,18865,6.32,AUTO
```

#### Status Updates
```
STATUS:MAGNET:ON_MAGNET    # Magnet state change
STATUS:MOUSE:ENTERED       # Mouse status change
STATUS:POSITION:5          # Position change
STATUS:HOMED:TRUE          # Homing complete
STATUS:SESSION:NEW         # New session started
STATUS:DOOR:OPEN           # Door opened
STATUS:DOOR:CLOSED         # Door closed
```

### PC → Arduino Commands

| Command | Description |
|---------|-------------|
| `home` | Initialize system and find home position |
| `p1` to `p12` | Move to specific position (1-12) |
| `open` | Manually open door (only when on magnet) |
| `close` | Manually close door (only when on magnet) |
| `stop` or `s` | Emergency stop motor |
| `status` | Display full system status |
| `mag` | Test magnetic sensors for 10 seconds |
| `beam` | Test beam breakers for 10 seconds |
| `rpm [1-30]` | Set target RPM speed |

## 🔧 Hardware Configuration

### Pin Assignments

```cpp
#define STEP_PIN 7         // Stepper pulse
#define DIR_PIN 8          // Stepper direction
#define ENABLE_PIN 4       // Stepper enable (LOW=enabled)
#define MAG1_PIN 9         // Home sensor (position 1)
#define MAG2_PIN 10        // Position sensors (2-12)
#define SERVO1_PIN 11      // Right door servo
#define SERVO2_PIN 12      // Left door servo
#define BEAM_S1_PIN A0     // Beam breaker mainchamber (inside)
#define BEAM_S2_PIN A1     // Beam breaker subchamber (outside)
```

### DM542T Driver Settings

**For 800 pulses/revolution:**

| Switch | Position | Function |
|--------|----------|----------|
| SW1-SW3 | OFF | Microstepping = 800 steps/rev |
| SW4 | OFF | Decay mode |
| SW5-SW8 | Varies | Current setting (see datasheet) |

**CRITICAL**: Set DM542T to 5V logic mode (internal jumper).

### Default Motor Settings

```cpp
RMS Current: 3.2A
Pulses/Revolution: 800
Target RPM: 10
Homing RPM: 8
Acceleration: 50 steps/sec²
```

## 📐 System Specifications

### Physical
- **Positions**: 12 evenly spaced around circle
- **Position Spacing**: 840 steps (hardcoded)
- **Total Revolution**: 10,080 steps (12 × 840)
- **Navigation**: Shortest path algorithm

### Timing
- **Entry Detection**: S1 break → S2 break
- **Exit Detection**: S2 break → S1 break  
- **Timestamp Resolution**: 1 millisecond (Arduino millis())
- **Dwell Time Precision**: 0.01 seconds (2 decimal places)

### Behavior
- **Automatic Door Cycle**: Triggered after position movement
- **Manual Door Control**: Available when on magnet
- **Trial Counter**: Resets on each `home` command
- **Session Management**: One session = one homing to next homing

## 🐛 Troubleshooting

### Arduino Issues

| Problem | Solution |
|---------|----------|
| Motor not moving | Check ENABLE_PIN (should be LOW), verify power supply |
| Wrong speed | Verify pulsePerRev matches driver DIP switches |
| Missed steps | Increase RMS current or decrease acceleration |
| Never homes | Check MAG1 sensor wiring, run `mag` test |
| Misses positions | Check MAG2 sensor placement, verify 840 steps |

### Python GUI Issues

| Problem | Solution |
|---------|----------|
| Port not found | Check USB connection, install CH340 drivers |
| Connection fails | Close Arduino IDE serial monitor, restart GUI |
| Data not logging | Verify firmware v1.4.0, complete full door cycle |
| Excel errors | Check write permissions, close Excel if file open |

### Data Quality Issues

| Problem | Solution |
|---------|----------|
| Dwell time = 0 | Mouse didn't fully enter (S2 never triggered) |
| Missing trials | Door cycle incomplete, check beam sensors |
| Incorrect timestamps | Arduino reset during experiment (millis() restarted) |

## 📚 Project Structure

```
/workspace/
├── Carousel_Controller/
│   ├── Carousel_Controller.ino    # v1.4.0 firmware
│   └── README.md                   # Arduino-specific docs
│
├── Python_GUI/
│   ├── carousel_gui.py             # Main GUI application
│   ├── serial_handler.py           # Serial communication
│   ├── data_logger.py              # Excel file handling
│   ├── requirements.txt            # Python dependencies
│   └── README.md                   # GUI usage guide
│
├── Unit_Experiments/               # Hardware test sketches
│   ├── Beam_Breaker/
│   ├── Servo/
│   ├── Stepper_Motor/
│   └── Stepper_Speed_Test/
│
├── data/                           # Excel logs (auto-created, gitignored)
│   ├── Carousel_110725.xlsx
│   └── Carousel_110825.xlsx
│
├── Carousel_Controller_Specification.md  # Complete spec
├── README.md                       # This file
└── .gitignore                      # Git ignore rules
```

## 🔬 Example Use Cases

### 1. Novel Object Recognition Test
- Place different objects at positions 1-12
- Run home command at start of session
- Navigate to each position automatically
- Record dwell time at each object
- Analyze preference patterns

### 2. Spatial Memory Test
- Train animal with specific positions
- Test retention by measuring dwell times
- Compare AUTO vs MANUAL door control effects
- Track learning curves over multiple sessions

### 3. Anxiety/Exploration Test
- Measure exploration time in novel environments
- Compare dwell times across different conditions
- Track hesitation patterns (entry attempts)
- Correlate with other behavioral measures

## 📈 Data Analysis Tips

### Excel Analysis
```excel
=AVERAGE(E2:E100)          # Average dwell time
=STDEV(E2:E100)            # Standard deviation
=COUNTIF(F2:F100,"AUTO")   # Count AUTO trials
=SUMIF(F2:F100,"MANUAL",E2:E100)  # Sum MANUAL dwell times
```

### Python Analysis
```python
import pandas as pd

# Load data
df = pd.read_excel('data/Carousel_110725.xlsx')

# Calculate statistics
avg_dwell = df['DwellTime'].mean()
std_dwell = df['DwellTime'].std()

# Filter by event type
auto_trials = df[df['Event'] == 'AUTO']
manual_trials = df[df['Event'] == 'MANUAL']

# Group by position
position_stats = df.groupby('Position')['DwellTime'].describe()
```

## 🤝 Contributing

### Reporting Issues
1. Check existing issues
2. Include firmware version
3. Attach communication log
4. Provide example data if relevant

### Feature Requests
- Describe use case
- Explain expected behavior
- Suggest implementation if possible

## 📄 License

See LICENSE file in repository.

## 👥 Authors

- **Firmware**: Arduino C++ (AccelStepper, Servo libraries)
- **GUI**: Python/tkinter (pyserial, pandas, openpyxl)
- **Hardware**: DM542T driver, NEMA 23 motor, Arduino UNO R4 Minima

## 📞 Support

For technical support:
1. Review this README and Python_GUI/README.md
2. Check Carousel_Controller_Specification.md for detailed specs
3. Run troubleshooting tests (`mag`, `beam` commands)
4. Review communication log for error messages

## 🎓 Citations

If using this system in research, please cite:
- Arduino AccelStepper library by Mike McCauley
- Hardware specifications in your methods section
- Software version (v1.4.0)

## ⚠️ Safety Notes

- **Motor Power**: Ensure proper current settings to prevent overheating
- **Emergency Stop**: Always available via GUI or serial command
- **Mechanical Safety**: Ensure no obstruction in carousel path
- **Electrical Safety**: Verify all connections before powering on
- **Animal Welfare**: Follow institutional guidelines for behavioral testing

## 🔄 Version History

### v1.4.0 (2025-11-07) - Current
- ✨ Added dwell time tracking with millisecond precision
- ✨ Excel data logging with date-based files
- ✨ Python GUI with 5 sections (connection, status, controls, storage, log)
- ✨ Real-time status updates (Magnet State, Mouse Status)
- ✨ SESSION management and trial counter
- ✨ AUTO/MANUAL event type tracking
- 📝 Enhanced serial protocol (DATA packets, STATUS updates)
- 🔧 Cleaned up enum definitions
- 📚 Comprehensive documentation

### v1.3.0 (Previous)
- Basic carousel control with 12 positions
- Automatic door cycle on position arrival
- Manual door open/close
- Beam break monitoring
- Serial command interface

---

**Ready to start tracking dwell times!** 🐭📊✨
