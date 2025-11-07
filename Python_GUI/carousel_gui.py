"""
Carousel Controller - Main GUI Application
Version: 1.4.0

Complete GUI interface for controlling the carousel and logging dwell time data.
Based on SOLAR_ControllerV3 reference design.
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, messagebox
import os
import subprocess
import platform
from datetime import datetime

from serial_handler import SerialHandler
from data_logger import DataLogger


class CarouselControlGUI:
    """
    Main GUI application for Carousel Controller.
    
    Features:
    - Serial connection management with auto-detection
    - Real-time system status display (Magnet State, Mouse Status)
    - Control buttons (Home, Position, Door, Tests)
    - Data storage location display
    - Communication log with color coding
    """
    
    def __init__(self, root):
        """Initialize the GUI application."""
        self.root = root
        self.root.title("Carousel Controller v1.4.0 - Dwell Time Logger")
        self.root.geometry("900x700")
        
        # Initialize backend components
        self.serial_handler = SerialHandler(self)
        self.data_logger = DataLogger()
        
        # State tracking
        self.auto_detect_enabled = tk.BooleanVar(value=True)
        
        # Build GUI sections
        self.create_section1_serial_connection()
        self.create_section2_system_status()
        self.create_section3_controls()
        self.create_section4_data_storage()
        self.create_section5_communication_log()
        
        # Configure grid weights for resizing
        self.root.grid_rowconfigure(3, weight=1)
        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_columnconfigure(1, weight=1)
        
        # Start port refresh timer
        self.refresh_ports()
        
    # ============================================
    # SECTION 1: Serial Connection
    # ============================================
    
    def create_section1_serial_connection(self):
        """Create serial connection controls."""
        frame = ttk.LabelFrame(self.root, text="Serial Connection", padding=10)
        frame.grid(row=0, column=0, columnspan=2, sticky="ew", padx=5, pady=5)
        
        # Port selection
        ttk.Label(frame, text="Port:").grid(row=0, column=0, padx=5)
        self.port_combo = ttk.Combobox(frame, width=15, state='readonly')
        self.port_combo.grid(row=0, column=1, padx=5)
        
        # Auto-detect checkbox
        self.auto_check = ttk.Checkbutton(frame, text="Auto-detect", 
                                          variable=self.auto_detect_enabled,
                                          command=self.on_auto_detect_toggle)
        self.auto_check.grid(row=0, column=2, padx=5)
        
        # Refresh button
        ttk.Button(frame, text="↻", width=3, 
                   command=self.refresh_ports).grid(row=0, column=3, padx=5)
        
        # Connect button
        self.connect_btn = ttk.Button(frame, text="Connect", 
                                      command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=4, padx=5)
        
        # Status indicator
        self.conn_status_label = ttk.Label(frame, text="●", foreground="red", font=("Arial", 16))
        self.conn_status_label.grid(row=0, column=5, padx=5)
        ttk.Label(frame, text="Disconnected").grid(row=0, column=6, padx=5)
        self.conn_text_label = ttk.Label(frame, text="Disconnected")
        self.conn_text_label.grid(row=0, column=6, padx=5)
    
    def refresh_ports(self):
        """Refresh available serial ports."""
        ports = self.serial_handler.get_available_ports()
        self.port_combo['values'] = ports
        
        if self.auto_detect_enabled.get():
            # Try to auto-detect Arduino
            arduino_port = self.serial_handler.auto_detect_arduino()
            if arduino_port and arduino_port in ports:
                self.port_combo.set(arduino_port)
            elif ports:
                self.port_combo.set(ports[0])
        elif not self.port_combo.get() and ports:
            self.port_combo.set(ports[0])
        
        # Schedule next refresh
        self.root.after(2000, self.refresh_ports)
    
    def on_auto_detect_toggle(self):
        """Handle auto-detect checkbox toggle."""
        if self.auto_detect_enabled.get():
            self.refresh_ports()
    
    def toggle_connection(self):
        """Toggle serial connection on/off."""
        if not self.serial_handler.is_connected:
            # Connect
            port = self.port_combo.get()
            if not port:
                messagebox.showerror("Error", "No port selected!")
                return
            
            if self.serial_handler.connect(port):
                self.connect_btn.config(text="Disconnect")
                self.conn_status_label.config(foreground="green")
                self.conn_text_label.config(text="Connected")
                self.log_message(f"Connected to {port}", "INFO")
            else:
                messagebox.showerror("Error", "Failed to connect!")
        else:
            # Disconnect
            self.serial_handler.disconnect()
            self.connect_btn.config(text="Connect")
            self.conn_status_label.config(foreground="red")
            self.conn_text_label.config(text="Disconnected")
            self.log_message("Disconnected", "INFO")
    
    # ============================================
    # SECTION 2: System Status
    # ============================================
    
    def create_section2_system_status(self):
        """Create system status display."""
        frame = ttk.LabelFrame(self.root, text="System Status", padding=10)
        frame.grid(row=1, column=0, sticky="nsew", padx=5, pady=5)
        
        # Magnet State
        ttk.Label(frame, text="Magnet State:", font=("Arial", 12)).grid(row=0, column=0, sticky="w", pady=5)
        self.magnet_label = ttk.Label(frame, text="Unknown", 
                                      font=("Arial", 12, "bold"), foreground="gray")
        self.magnet_label.grid(row=0, column=1, sticky="w", pady=5, padx=10)
        
        # Mouse Status (renamed from Beam State)
        ttk.Label(frame, text="Mouse Status:", font=("Arial", 12)).grid(row=1, column=0, sticky="w", pady=5)
        self.mouse_label = ttk.Label(frame, text="IDLE", 
                                     font=("Arial", 12, "bold"), foreground="light blue")
        self.mouse_label.grid(row=1, column=1, sticky="w", pady=5, padx=10)
        
        # Current Position
        ttk.Label(frame, text="Position:", font=("Arial", 12)).grid(row=2, column=0, sticky="w", pady=5)
        self.position_label = ttk.Label(frame, text="Unknown", 
                                       font=("Arial", 12, "bold"))
        self.position_label.grid(row=2, column=1, sticky="w", pady=5, padx=10)
        
        # Buttons
        btn_frame = ttk.Frame(frame)
        btn_frame.grid(row=3, column=0, columnspan=2, pady=10)
        
        ttk.Button(btn_frame, text="Device Status", 
                   command=self.send_status_command).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Emergency Stop", 
                   command=self.send_stop_command).pack(side="left", padx=5)
    
    def send_status_command(self):
        """Send status command to Arduino."""
        self.serial_handler.send_command("status")
    
    def send_stop_command(self):
        """Send emergency stop command to Arduino."""
        if messagebox.askyesno("Confirm", "Emergency stop the motor?"):
            self.serial_handler.send_command("stop")
    
    # ============================================
    # SECTION 3: Controls
    # ============================================
    
    def create_section3_controls(self):
        """Create control buttons."""
        frame = ttk.LabelFrame(self.root, text="Controls", padding=10)
        frame.grid(row=1, column=1, sticky="nsew", padx=5, pady=5)
        
        # Home button
        ttk.Button(frame, text="Home", command=self.send_home, 
                   width=15).grid(row=0, column=0, columnspan=2, pady=5)
        
        # Position control
        ttk.Label(frame, text="Position:").grid(row=1, column=0, sticky="w", pady=5)
        self.position_combo = ttk.Combobox(frame, 
                                          values=[f"p{i}" for i in range(1, 13)],
                                          width=8, state='readonly')
        self.position_combo.grid(row=1, column=1, pady=5)
        self.position_combo.set("p1")
        ttk.Button(frame, text="Go", command=self.send_position,
                   width=6).grid(row=1, column=2, pady=5, padx=5)
        
        # Manual door control
        door_frame = ttk.LabelFrame(frame, text="Manual Door", padding=5)
        door_frame.grid(row=2, column=0, columnspan=3, pady=10, sticky="ew")
        
        ttk.Button(door_frame, text="Open", command=self.send_open,
                   width=10).pack(side="left", padx=5)
        ttk.Button(door_frame, text="Close", command=self.send_close,
                   width=10).pack(side="left", padx=5)
        
        # Troubleshooting
        trouble_frame = ttk.LabelFrame(frame, text="Troubleshooting", padding=5)
        trouble_frame.grid(row=3, column=0, columnspan=3, pady=10, sticky="ew")
        
        ttk.Button(trouble_frame, text="Test Mag", command=self.send_mag,
                   width=10).pack(side="left", padx=5)
        ttk.Button(trouble_frame, text="Test Beam", command=self.send_beam,
                   width=10).pack(side="left", padx=5)
    
    def send_home(self):
        """Send home command."""
        self.serial_handler.send_command("home")
    
    def send_position(self):
        """Send position command."""
        pos = self.position_combo.get()
        if pos:
            self.serial_handler.send_command(pos)
    
    def send_open(self):
        """Send manual door open command."""
        self.serial_handler.send_command("open")
    
    def send_close(self):
        """Send manual door close command."""
        self.serial_handler.send_command("close")
    
    def send_mag(self):
        """Send mag test command."""
        self.serial_handler.send_command("mag")
    
    def send_beam(self):
        """Send beam test command."""
        self.serial_handler.send_command("beam")
    
    # ============================================
    # SECTION 4: Data Storage
    # ============================================
    
    def create_section4_data_storage(self):
        """Create data storage information display."""
        frame = ttk.LabelFrame(self.root, text="Data Storage", padding=10)
        frame.grid(row=2, column=0, columnspan=2, sticky="ew", padx=5, pady=5)
        
        # Current file display
        ttk.Label(frame, text="Current File:").grid(row=0, column=0, sticky="w")
        self.file_label = ttk.Label(frame, text=self.data_logger.get_current_filename(), 
                                    font=("Arial", 12, "bold"))
        self.file_label.grid(row=0, column=1, sticky="w", padx=10)
        
        # Trial count
        ttk.Label(frame, text="Trials Today:").grid(row=0, column=2, sticky="w", padx=10)
        self.trial_count_label = ttk.Label(frame, text="0", font=("Arial", 12))
        self.trial_count_label.grid(row=0, column=3, sticky="w")
        
        # Location
        ttk.Label(frame, text="Location:").grid(row=1, column=0, sticky="w")
        location_text = str(self.data_logger.data_folder.absolute())
        self.location_label = ttk.Label(frame, text=location_text, font=("Courier", 8))
        self.location_label.grid(row=1, column=1, columnspan=2, sticky="w", padx=10)
        
        # Open folder button
        ttk.Button(frame, text="Open Folder", 
                   command=self.open_data_folder).grid(row=1, column=3, padx=5)
        
        # Update file display every 5 seconds
        self.update_file_display()
    
    def update_file_display(self):
        """Update file display with current information."""
        filename = self.data_logger.get_current_filename()
        trial_count = self.data_logger.get_trial_count()
        
        self.file_label.config(text=filename)
        self.trial_count_label.config(text=str(trial_count))
        
        # Schedule next update
        self.root.after(5000, self.update_file_display)
    
    def open_data_folder(self):
        """Open data folder in file explorer."""
        folder_path = self.data_logger.get_data_folder_path()
        try:
            if platform.system() == "Windows":
                os.startfile(folder_path)
            elif platform.system() == "Darwin":  # macOS
                subprocess.Popen(["open", folder_path])
            else:  # Linux
                subprocess.Popen(["xdg-open", folder_path])
        except Exception as e:
            messagebox.showerror("Error", f"Could not open folder: {e}")
    
    # ============================================
    # SECTION 5: Communication Log
    # ============================================
    
    def create_section5_communication_log(self):
        """Create communication log display."""
        frame = ttk.LabelFrame(self.root, text="Communication Log", padding=10)
        frame.grid(row=3, column=0, columnspan=2, sticky="nsew", padx=5, pady=5)
        
        # Scrollable text widget
        self.log_text = scrolledtext.ScrolledText(frame, height=15, width=100, wrap="word",
                                                   font=("Courier", 9))
        self.log_text.grid(row=0, column=0, sticky="nsew")
        
        # Configure tags for color coding
        self.log_text.tag_config("INFO", foreground="black")
        self.log_text.tag_config("WARNING", foreground="orange")
        self.log_text.tag_config("ERROR", foreground="red")
        self.log_text.tag_config("DATA", foreground="blue", font=("Courier", 9, "bold"))
        self.log_text.tag_config("STATUS", foreground="green")
        self.log_text.tag_config("COMMAND", foreground="purple")
        
        # Buttons
        btn_frame = ttk.Frame(frame)
        btn_frame.grid(row=1, column=0, pady=5)
        ttk.Button(btn_frame, text="Clear Log", 
                   command=self.clear_log).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Save Log", 
                   command=self.save_log).pack(side="left", padx=5)
        
        # Configure grid weights
        frame.grid_rowconfigure(0, weight=1)
        frame.grid_columnconfigure(0, weight=1)
        
        # Initial log message
        self.log_message("=== Carousel Controller v1.4.0 Started ===", "INFO")
        self.log_message(f"Data folder: {self.data_logger.get_data_folder_path()}", "INFO")
    
    def log_message(self, message, message_type="INFO"):
        """
        Add message to communication log with timestamp and color coding.
        
        Args:
            message (str): Message to log
            message_type (str): Type of message (INFO, WARNING, ERROR, DATA, STATUS, COMMAND)
        """
        timestamp = datetime.now().strftime("%H:%M:%S")
        formatted_message = f"[{timestamp}] {message}\n"
        
        self.log_text.insert("end", formatted_message, message_type)
        self.log_text.see("end")  # Auto-scroll to bottom
    
    def clear_log(self):
        """Clear the communication log."""
        if messagebox.askyesno("Confirm", "Clear communication log?"):
            self.log_text.delete("1.0", "end")
            self.log_message("Log cleared", "INFO")
    
    def save_log(self):
        """Save communication log to file."""
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
            initialfile=f"carousel_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        )
        if filename:
            try:
                with open(filename, 'w') as f:
                    f.write(self.log_text.get("1.0", "end"))
                messagebox.showinfo("Success", f"Log saved to {filename}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save log: {e}")
    
    # ============================================
    # Data Handling Callbacks
    # ============================================
    
    def handle_data_packet(self, line):
        """
        Handle DATA packet from Arduino.
        
        Args:
            line (str): DATA packet line
        """
        success = self.data_logger.parse_data_packet(line)
        if success:
            self.log_message(f"✓ Data logged successfully", "STATUS")
            # Update trial count
            self.trial_count_label.config(text=str(self.data_logger.get_trial_count()))
        else:
            self.log_message(f"✗ Failed to log data", "ERROR")
    
    def handle_status_update(self, line):
        """
        Handle STATUS update from Arduino.
        
        Format: STATUS:FIELD:VALUE
        Examples:
            STATUS:MAGNET:ON_MAGNET
            STATUS:MOUSE:ENTERED
            STATUS:POSITION:5
        
        Args:
            line (str): STATUS update line
        """
        try:
            parts = line.split(':')
            if len(parts) == 3:
                field = parts[1]
                value = parts[2]
                
                if field == "MAGNET":
                    self.magnet_label.config(text=value)
                    # Color coding
                    if value == "ON_MAGNET":
                        self.magnet_label.config(foreground="green")
                    else:
                        self.magnet_label.config(foreground="gray")
                
                elif field == "MOUSE":
                    self.mouse_label.config(text=value)
                    # Color coding for mouse status
                    if value == "IDLE":
                        self.mouse_label.config(foreground="blue")
                    elif value == "ENTRY":
                        self.mouse_label.config(foreground="orange")
                    elif value == "ENTERED":
                        self.mouse_label.config(foreground="green")
                    elif value == "EXIT":
                        self.mouse_label.config(foreground="orange")
                
                elif field == "POSITION":
                    if value != "0":
                        self.position_label.config(text=f"p{value}")
                    else:
                        self.position_label.config(text="Unknown")
                        
        except Exception as e:
            self.log_message(f"Error parsing status update: {e}", "ERROR")


def main():
    """Main entry point for the application."""
    root = tk.Tk()
    app = CarouselControlGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
