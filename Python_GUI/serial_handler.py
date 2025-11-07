"""
Carousel Controller - Serial Handler Module
Version: 1.4.0

Manages serial communication with Arduino.
Handles connection, disconnection, reading, and command sending.
"""

import serial
import serial.tools.list_ports
import threading
import time


class SerialHandler:
    """
    Manages serial communication with Arduino.
    
    Features:
    - Auto-detection of available serial ports
    - Non-blocking serial reading via threading
    - Line parsing and routing (DATA, STATUS, ERROR)
    - Command sending
    - Connection state management
    """
    
    def __init__(self, gui):
        """
        Initialize serial handler.
        
        Args:
            gui: Reference to GUI object for callbacks
        """
        self.gui = gui
        self.serial_port = None
        self.is_connected = False
        self.read_thread = None
        self.running = False
        
    def get_available_ports(self):
        """
        Get list of available serial ports.
        
        Returns:
            list: List of port device names (e.g., ['COM3', '/dev/ttyUSB0'])
        """
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]
    
    def auto_detect_arduino(self):
        """
        Attempt to auto-detect Arduino port.
        
        Returns:
            str or None: Port name if Arduino found, None otherwise
        """
        ports = serial.tools.list_ports.comports()
        for port in ports:
            # Look for common Arduino identifiers
            if 'Arduino' in port.description or 'CH340' in port.description or \
               'USB' in port.description or 'ACM' in port.device:
                return port.device
        return None
    
    def connect(self, port_name, baudrate=115200):
        """
        Connect to Arduino on specified port.
        
        Args:
            port_name (str): Serial port name (e.g., 'COM3', '/dev/ttyUSB0')
            baudrate (int): Baud rate (default: 115200)
            
        Returns:
            bool: True if connected successfully, False otherwise
        """
        try:
            self.serial_port = serial.Serial(port_name, baudrate, timeout=0.1)
            time.sleep(2)  # Wait for Arduino reset after connection
            self.is_connected = True
            self.start_reading()
            return True
        except Exception as e:
            self.gui.log_message(f"Connection error: {e}", "ERROR")
            return False
    
    def disconnect(self):
        """Disconnect from Arduino."""
        self.running = False
        if self.read_thread:
            self.read_thread.join(timeout=2)
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.is_connected = False
    
    def start_reading(self):
        """Start background thread for reading serial data."""
        self.running = True
        self.read_thread = threading.Thread(target=self._read_loop, daemon=True)
        self.read_thread.start()
    
    def _read_loop(self):
        """Background loop to continuously read serial data."""
        buffer = ""
        while self.running and self.serial_port and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting:
                    # Read available bytes
                    chunk = self.serial_port.read(self.serial_port.in_waiting).decode('utf-8', errors='ignore')
                    buffer += chunk
                    
                    # Process complete lines
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        if line:
                            self.process_line(line)
                            
            except Exception as e:
                self.gui.log_message(f"Read error: {e}", "ERROR")
                time.sleep(0.1)
            time.sleep(0.01)  # Small delay to prevent CPU hogging
    
    def process_line(self, line):
        """
        Parse and route incoming serial line.
        
        Args:
            line (str): Received line from Arduino
        """
        if line.startswith("DATA,"):
            # Data packet - send to data logger and GUI
            self.gui.handle_data_packet(line)
            self.gui.log_message(line, "DATA")
            
        elif line.startswith("STATUS:"):
            # Status update - send to GUI
            self.gui.handle_status_update(line)
            self.gui.log_message(line, "STATUS")
            
        elif line.startswith("ERROR:"):
            # Error message
            self.gui.log_message(line, "ERROR")
            
        elif "WARNING" in line or "⚠️" in line:
            # Warning message
            self.gui.log_message(line, "WARNING")
            
        else:
            # General information
            self.gui.log_message(line, "INFO")
    
    def send_command(self, command):
        """
        Send command to Arduino.
        
        Args:
            command (str): Command to send (without newline)
            
        Returns:
            bool: True if sent successfully, False otherwise
        """
        if self.is_connected and self.serial_port:
            try:
                self.serial_port.write(f"{command}\n".encode('utf-8'))
                self.gui.log_message(f">> {command}", "COMMAND")
                return True
            except Exception as e:
                self.gui.log_message(f"Send error: {e}", "ERROR")
                return False
        else:
            self.gui.log_message("ERROR: Not connected to Arduino", "ERROR")
            return False
    
    def is_open(self):
        """
        Check if serial port is open.
        
        Returns:
            bool: True if port is open, False otherwise
        """
        return self.serial_port and self.serial_port.is_open if self.serial_port else False
