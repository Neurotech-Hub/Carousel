# Carousel Controller v1.4.0 - Testing Checklist

Use this checklist to verify the complete implementation is working correctly.

## 🔧 Prerequisites

- [ ] Arduino UNO R4 Minima connected via USB
- [ ] DM542T driver powered and configured
- [ ] All sensors connected (MAG1, MAG2, BEAM_S1, BEAM_S2)
- [ ] Servos connected and functional
- [ ] Python 3.7+ installed
- [ ] Python packages installed: `pip install -r Python_GUI/requirements.txt`

---

## 📱 Arduino Firmware Tests

### Upload & Basic Communication

- [ ] Open Arduino IDE
- [ ] Open `Carousel_Controller/Carousel_Controller.ino`
- [ ] Verify version shows "1.4.0" at top of file
- [ ] Upload to Arduino UNO R4 Minima
- [ ] Open Serial Monitor (115200 baud)
- [ ] Verify startup banner shows "=== Carousel Controller 1.4.0 ==="
- [ ] Verify default configuration displays
- [ ] See "⚠️ IMPORTANT: Run 'home' command first"

### Homing Test

- [ ] Send command: `home`
- [ ] Motor starts rotating
- [ ] Motor stops when MAG1 detected
- [ ] See "HOMING COMPLETE" message
- [ ] See "New session started - trial counter reset"
- [ ] See status updates: `STATUS:SESSION:NEW`, `STATUS:MAGNET:ON_MAGNET`, `STATUS:MOUSE:IDLE`

### Position Movement Test

- [ ] Send command: `p5`
- [ ] Motor moves to position 5
- [ ] See "Arrived at Box 5 - Sensor detected ✓"
- [ ] Door opens automatically
- [ ] See "Opening door..."
- [ ] See "Beam monitoring active"
- [ ] See `STATUS:DOOR:OPEN`

### Beam Monitoring Test

- [ ] With door open, manually block S1 sensor (mainchamber)
- [ ] See `STATUS:MOUSE:ENTRY`
- [ ] Then block S2 sensor (subchamber)
- [ ] See "Mouse in subchamber"
- [ ] See `STATUS:MOUSE:ENTERED`
- [ ] Unblock sensors
- [ ] Block S2 again (exit sequence start)
- [ ] See `STATUS:MOUSE:EXIT`
- [ ] Block S1 again (return to mainchamber)
- [ ] See "Mouse returned - closing door"
- [ ] Door closes automatically
- [ ] See DATA packet: `DATA,1,5,<entry_time>,<exit_time>,<dwell_time>,AUTO`

### Manual Door Test

- [ ] Send command: `open`
- [ ] Door opens manually
- [ ] Repeat beam sequence (S1, S2, S2, S1)
- [ ] Send command: `close`
- [ ] Door closes manually
- [ ] See DATA packet with "MANUAL" event type
- [ ] Trial number incremented

### Status Command Test

- [ ] Send command: `status`
- [ ] See complete system status
- [ ] See "--- STRUCTURED STATUS ---" section
- [ ] Verify STATUS updates for: MAGNET, MOUSE, POSITION, HOMED, TRIAL

### Sensor Tests

- [ ] Send command: `mag`
- [ ] Bring magnet near MAG1 sensor
- [ ] See "Home sensor detected!"
- [ ] Bring magnet near MAG2 sensor
- [ ] See "Position sensor detected!"
- [ ] Remove magnet
- [ ] See "No sensor detected"

- [ ] Send command: `beam`
- [ ] Block S1 beam breaker
- [ ] See "S1=<value> BLOCKED"
- [ ] Clear S1
- [ ] See "S1=<value> CLEAR"
- [ ] Block S2 beam breaker
- [ ] See "S2=<value> BLOCKED"

### Emergency Stop Test

- [ ] Send command to move (e.g., `p8`)
- [ ] While moving, send: `stop`
- [ ] Motor decelerates and stops
- [ ] See "Motor stopped by user."

---

## 🖥️ Python GUI Tests

### Startup Tests

- [ ] Navigate to `Python_GUI/` folder
- [ ] Run: `python carousel_gui.py`
- [ ] GUI window opens with title "Carousel Controller v1.4.0"
- [ ] All 5 sections visible:
  - [ ] Serial Connection
  - [ ] System Status
  - [ ] Controls
  - [ ] Data Storage
  - [ ] Communication Log
- [ ] See startup message in Communication Log
- [ ] See data folder path displayed

### Serial Connection Tests

- [ ] Port dropdown populated with available ports
- [ ] Auto-detect checkbox enabled by default
- [ ] If Arduino connected, port auto-selected
- [ ] Click "Connect" button
- [ ] Wait 2 seconds for Arduino reset
- [ ] Status indicator turns GREEN
- [ ] Status text shows "Connected"
- [ ] Connection message appears in log
- [ ] Click "Disconnect" button
- [ ] Status indicator turns RED
- [ ] Status text shows "Disconnected"
- [ ] Reconnect for remaining tests

### System Status Display Tests

- [ ] Send "home" command via Arduino IDE or GUI
- [ ] Magnet State changes to "ON_MAGNET" (green)
- [ ] Mouse Status shows "IDLE" (blue)
- [ ] Position shows "p1"

- [ ] Click "Device Status" button
- [ ] `status` command sent
- [ ] Status values update in GUI

- [ ] Test Mouse Status color changes:
  - [ ] IDLE = blue
  - [ ] ENTRY = orange
  - [ ] ENTERED = green
  - [ ] EXIT = orange

### Controls Tests

#### Home Button
- [ ] Click "Home" button
- [ ] See command in Communication Log: `>> home`
- [ ] Arduino performs homing sequence
- [ ] Trial counter resets to 0 (check Data Storage section)

#### Position Control
- [ ] Select "p3" from dropdown
- [ ] Click "Go" button
- [ ] See command: `>> p3`
- [ ] Motor moves to position 3
- [ ] Door opens automatically
- [ ] Mouse Status updates during beam sequence

#### Manual Door Control
- [ ] At position, click "Open" button
- [ ] Door opens
- [ ] Beam monitoring starts
- [ ] Click "Close" button
- [ ] Door closes
- [ ] DATA packet appears with MANUAL

#### Troubleshooting Buttons
- [ ] Click "Test Mag" button
- [ ] Sensor test runs for 10 seconds
- [ ] Results appear in Communication Log

- [ ] Click "Test Beam" button
- [ ] Beam test runs for 10 seconds
- [ ] S1 and S2 values displayed

#### Emergency Stop Button
- [ ] Send position command (e.g., p7)
- [ ] While moving, click "Emergency Stop"
- [ ] Confirmation dialog appears
- [ ] Click "Yes"
- [ ] Motor stops
- [ ] Stop command sent

### Data Storage Tests

- [ ] Check "Current File" shows today's date format: `Carousel_MMDDYY.xlsx`
- [ ] "Trials Today" shows "0" initially
- [ ] Perform one complete trial (position → door cycle → data saved)
- [ ] "Trials Today" increments to "1"
- [ ] "Location" shows full path to data folder
- [ ] Click "Open Folder" button
- [ ] File explorer/finder opens to data folder
- [ ] Excel file exists with correct name

### Communication Log Tests

- [ ] Messages appear with timestamps `[HH:MM:SS]`
- [ ] Color coding correct:
  - [ ] Black = INFO messages
  - [ ] Purple = Commands (starting with >>)
  - [ ] Blue bold = DATA packets
  - [ ] Green = STATUS updates
  - [ ] Orange = Warnings
  - [ ] Red = Errors

- [ ] Log auto-scrolls to bottom
- [ ] Click "Clear Log" button
- [ ] Confirmation dialog appears
- [ ] Click "Yes"
- [ ] Log clears (except for "Log cleared" message)

- [ ] Add some log messages
- [ ] Click "Save Log" button
- [ ] File save dialog appears
- [ ] Save to file
- [ ] Open saved file
- [ ] Verify all log content present

---

## 📊 Data Logging Tests

### Excel File Creation

- [ ] Ensure no Excel file exists for today
- [ ] Perform one complete trial
- [ ] Navigate to `data/` folder
- [ ] Excel file created: `Carousel_MMDDYY.xlsx`
- [ ] Open Excel file
- [ ] Verify column headers:
  - Trial, Position, EntryTime, ExitTime, DwellTime, Event, Timestamp
- [ ] Verify one data row present

### Data Accuracy Test

- [ ] Note Arduino entry time from serial output
- [ ] Note Arduino exit time from serial output
- [ ] Open Excel file
- [ ] Verify Entry and Exit times match (within 100ms)
- [ ] Calculate dwell manually: (exit - entry) / 1000
- [ ] Verify DwellTime column matches calculation (within 0.01s)
- [ ] Verify Position column is correct
- [ ] Verify Event column shows "AUTO" or "MANUAL" correctly

### Auto vs Manual Test

- [ ] Perform AUTO trial (use position command)
- [ ] Check Excel: Event column = "AUTO"
- [ ] Perform MANUAL trial (use open/close buttons)
- [ ] Check Excel: Event column = "MANUAL"

### Daily File Rollover Test

- [ ] Note current filename
- [ ] (Optional) Change system date to tomorrow
- [ ] Restart GUI
- [ ] Perform a trial
- [ ] New Excel file created with new date
- [ ] Both files present in data folder

### File Append Test

- [ ] Note current trial count (e.g., 5)
- [ ] Close GUI
- [ ] Restart GUI
- [ ] Perform another trial
- [ ] Open Excel file
- [ ] Trial numbers continue sequentially (e.g., 6, 7, 8...)
- [ ] All previous data still present

---

## 🔄 Integration Tests

### Complete Workflow Test

- [ ] **Setup**:
  - [ ] Connect Arduino
  - [ ] Open GUI
  - [ ] Connect to Arduino
  - [ ] Send Home command

- [ ] **Trial 1 (AUTO)**:
  - [ ] Select p2
  - [ ] Click Go
  - [ ] Wait for arrival
  - [ ] Door opens automatically
  - [ ] Simulate mouse entry (S1, S2)
  - [ ] See "Mouse in subchamber"
  - [ ] Wait 5 seconds (dwell time)
  - [ ] Simulate exit (S2, S1)
  - [ ] Door closes automatically
  - [ ] DATA packet appears
  - [ ] Excel updated
  - [ ] Trial count = 1

- [ ] **Trial 2 (MANUAL)**:
  - [ ] Select p6
  - [ ] Click Go
  - [ ] Wait for arrival
  - [ ] Wait 2 seconds
  - [ ] Click Close (cancel AUTO)
  - [ ] Click Open (MANUAL)
  - [ ] Simulate entry/exit
  - [ ] Click Close
  - [ ] DATA packet with MANUAL
  - [ ] Trial count = 2

- [ ] **Trial 3 (Different Position)**:
  - [ ] Select p10
  - [ ] Click Go
  - [ ] Complete trial
  - [ ] Trial count = 3

- [ ] **Review Data**:
  - [ ] Open Excel file
  - [ ] 3 rows of data
  - [ ] Positions: 2, 6, 10
  - [ ] Events: AUTO, MANUAL, AUTO
  - [ ] Dwell times reasonable
  - [ ] Timestamps sequential

### Session Reset Test

- [ ] Note current trial number (e.g., 3)
- [ ] Click Home button
- [ ] Trial counter resets to 0 in GUI
- [ ] Perform new trial
- [ ] Trial number in DATA packet = 1
- [ ] Excel file continues appending (trials 4, 5, 6... as row numbers)

### Error Recovery Test

- [ ] Disconnect Arduino USB cable
- [ ] Try to send command
- [ ] Error message appears
- [ ] Reconnect USB cable
- [ ] Click Disconnect in GUI
- [ ] Click Connect in GUI
- [ ] Commands work again

### Data Integrity Test

- [ ] Perform 5 trials rapidly
- [ ] Check Excel file after each trial
- [ ] All 5 trials recorded
- [ ] No data loss
- [ ] Trial numbers sequential
- [ ] All timestamps present

---

## ✅ Success Criteria

All tests should pass with the following outcomes:

### Arduino Firmware:
- ✅ Homing works reliably
- ✅ Position movements accurate
- ✅ Beam detection correctly identifies entry/exit
- ✅ DATA packets formatted correctly
- ✅ STATUS updates sent appropriately
- ✅ Trial counter increments correctly
- ✅ Session resets on home command

### Python GUI:
- ✅ Connects to Arduino successfully
- ✅ All buttons functional
- ✅ Status displays update in real-time
- ✅ Communication log shows all messages
- ✅ Color coding correct
- ✅ No crashes or freezes

### Data Logging:
- ✅ Excel files created automatically
- ✅ Data saves after each trial
- ✅ Date-based naming works
- ✅ File appending works
- ✅ Data accuracy within ±100ms
- ✅ AUTO/MANUAL event types correct
- ✅ No data loss

---

## 🐛 If Tests Fail

### Arduino Issues:
1. Check firmware version (should be 1.4.0)
2. Verify AccelStepper and Servo libraries installed
3. Check wiring connections
4. Test sensors individually with `mag` and `beam` commands

### GUI Issues:
1. Verify Python packages installed: `pip list | grep -E "pyserial|openpyxl|pandas"`
2. Check Python version: `python --version` (should be 3.7+)
3. Review Communication Log for error messages
4. Restart GUI application

### Data Issues:
1. Check write permissions on `data/` folder
2. Close Excel if file is open
3. Verify disk space available
4. Check Communication Log for logging errors

---

## 📋 Test Results Template

```
=== TESTING RESULTS ===
Date: ____________
Tester: ____________

Arduino Firmware Tests:    ☐ PASS  ☐ FAIL
Python GUI Tests:          ☐ PASS  ☐ FAIL
Data Logging Tests:        ☐ PASS  ☐ FAIL
Integration Tests:         ☐ PASS  ☐ FAIL

Notes:
_______________________________________
_______________________________________
_______________________________________

Overall Status:            ☐ READY FOR PRODUCTION  ☐ NEEDS FIXES
```

---

**Complete all tests before using in actual experiments!** 🧪✅
