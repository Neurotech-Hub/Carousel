# Carousel Controller GUI v1.4.0

Python GUI application for controlling the Carousel Controller and logging rat dwell time data to Excel.

## Features

- **Serial Connection Management**: Auto-detection and manual port selection
- **Real-time Status Display**: Magnet state and mouse status monitoring
- **Position Control**: Move to any of 12 positions via GUI
- **Manual Door Control**: Open/close door with manual override
- **Data Logging**: Automatic dwell time recording to date-based Excel files
- **Communication Log**: Color-coded serial message display
- **Troubleshooting Tools**: Magnetic sensor and beam breaker testing

## Installation

### Prerequisites

- Python 3.7 or higher
- Arduino UNO R4 Minima with Carousel Controller firmware v1.4.0

### Install Dependencies

```bash
pip install -r requirements.txt
```

Or install individually:

```bash
pip install pyserial openpyxl pandas
```

## Usage

### Starting the GUI

```bash
python carousel_gui.py
```

Or on some systems:

```bash
python3 carousel_gui.py
```

### Quick Start Guide

1. **Connect Arduino**
   - Plug in Arduino UNO R4 Minima via USB
   - Select port from dropdown (or enable auto-detect)
   - Click "Connect" button
   - Wait for green indicator

2. **Initialize System**
   - Click "Home" button
   - Wait for homing to complete
   - System is ready when "HOMED ✓" appears

3. **Run Experiment**
   - Select position (p1-p12) from dropdown
   - Click "Go" button
   - Door opens automatically
   - Mouse enters/exits subchamber
   - Door closes automatically
   - Data logged to Excel

4. **View Data**
   - Click "Open Folder" to view Excel files
   - Files named: `Carousel_MMDDYY.xlsx`
   - New file created daily

## File Structure

```
Python_GUI/
├── carousel_gui.py       # Main GUI application
├── serial_handler.py     # Serial communication manager
├── data_logger.py        # Excel file handler
├── requirements.txt      # Python dependencies
└── README.md            # This file
```

## GUI Sections

### 1. Serial Connection
- Port selection dropdown
- Auto-detect checkbox
- Refresh button
- Connect/Disconnect button
- Connection status indicator

### 2. System Status
- Magnet State: ON_MAGNET / UNKNOWN
- Mouse Status: IDLE / ENTRY / ENTERED / EXIT
- Current Position: p1-p12
- Device Status button
- Emergency Stop button

### 3. Controls
- **Home**: Initialize system and reset trial counter
- **Position**: Select and move to position (p1-p12)
- **Manual Door**: Open/Close buttons
- **Troubleshooting**: Test Mag / Test Beam buttons

### 4. Data Storage
- Current Excel file name
- Number of trials today
- Data folder location
- Open Folder button

### 5. Communication Log
- Color-coded serial messages:
  - **Black**: General information
  - **Purple**: Commands sent
  - **Blue**: Data packets
  - **Green**: Status updates
  - **Orange**: Warnings
  - **Red**: Errors
- Clear Log button
- Save Log button

## Data Format

Excel files contain the following columns:

| Column | Description |
|--------|-------------|
| Trial | Sequential trial number (resets on home) |
| Position | Position number (1-12) |
| EntryTime | Arduino millis() when mouse entered |
| ExitTime | Arduino millis() when mouse exited |
| DwellTime | Time spent in subchamber (seconds) |
| Event | Door open type (AUTO or MANUAL) |
| Timestamp | PC timestamp when data was saved |

## Troubleshooting

### Cannot find serial port
- Check USB connection
- Install CH340 drivers if needed (for some Arduino clones)
- Try refreshing port list
- Manually select port if auto-detect fails

### Connection refused
- Close Arduino IDE serial monitor
- Close other serial terminal programs
- Restart application

### Data not logging
- Check that Arduino firmware is v1.4.0
- Verify door cycle completes (open → entry → exit → close)
- Check communication log for errors

### Excel file errors
- Ensure `./data` folder has write permissions
- Close Excel if file is open
- Check disk space

## Command Reference

### Arduino Commands (sent via GUI)

| Command | Description |
|---------|-------------|
| `home` | Find home position and initialize |
| `p1` to `p12` | Move to specific position |
| `open` | Manually open door |
| `close` | Manually close door |
| `stop` | Emergency stop motor |
| `status` | Display system status |
| `mag` | Test magnetic sensors (10 seconds) |
| `beam` | Test beam breaker sensors (10 seconds) |

## Technical Details

### Serial Protocol

**Data Packets (Arduino → PC):**
```
DATA,Trial,Position,EntryTime,ExitTime,DwellTime,Event
Example: DATA,1,5,12543,18865,6.32,AUTO
```

**Status Updates (Arduino → PC):**
```
STATUS:FIELD:VALUE
Examples:
  STATUS:MAGNET:ON_MAGNET
  STATUS:MOUSE:ENTERED
  STATUS:POSITION:5
```

### Threading

The application uses background threading for:
- Non-blocking serial port reading
- Real-time GUI updates
- Automatic port refreshing

## Support

For issues or questions:
1. Check Arduino firmware version (should be v1.4.0)
2. Verify all dependencies installed correctly
3. Check communication log for error messages
4. Review Arduino serial output for debugging

## Version History

### v1.4.0 (Current)
- Added dwell time tracking and Excel logging
- Real-time status updates (Magnet State, Mouse Status)
- Date-based Excel file management
- Enhanced communication log with color coding
- Data storage location display

### v1.3.0 (Previous)
- Basic carousel control
- Manual door operation
- Position navigation

## License

See main project LICENSE file.
