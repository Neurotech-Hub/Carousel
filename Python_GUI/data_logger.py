"""
Carousel Controller - Data Logger Module
Version: 1.4.0

Handles Excel file operations for dwell time data logging.
Creates/appends data to date-named Excel files (Carousel_MMDDYY.xlsx).
"""

import pandas as pd
import os
from datetime import datetime
from pathlib import Path


class DataLogger:
    """
    Manages Excel file operations for carousel dwell time data.
    
    Features:
    - Auto-creates date-based Excel files (Carousel_MMDDYY.xlsx)
    - Appends data to existing files on same date
    - Stores Arduino timestamps and PC timestamps
    - Validates and parses DATA packets
    """
    
    def __init__(self, data_folder="./data"):
        """
        Initialize data logger.
        
        Args:
            data_folder: Path to data storage directory (default: ./data)
        """
        self.data_folder = Path(data_folder)
        self.data_folder.mkdir(exist_ok=True)  # Create if doesn't exist
        self.current_file = None
        self.current_date = None
        self.update_file_path()
    
    def update_file_path(self):
        """Update file path based on current date."""
        today = datetime.now().strftime("%m%d%y")
        if today != self.current_date:
            self.current_date = today
            self.current_file = self.data_folder / f"Carousel_{today}.xlsx"
    
    def get_current_filename(self):
        """
        Get current filename.
        
        Returns:
            str: Current Excel filename (e.g., 'Carousel_110725.xlsx')
        """
        self.update_file_path()
        return self.current_file.name
    
    def get_current_filepath(self):
        """
        Get full path to current file.
        
        Returns:
            Path: Full path to current Excel file
        """
        self.update_file_path()
        return self.current_file
    
    def log_data(self, trial, position, entry_time, exit_time, dwell_time, event):
        """
        Log trial data to Excel file.
        
        Args:
            trial (int): Trial number
            position (int): Position number (1-12)
            entry_time (int): Arduino millis() when mouse entered
            exit_time (int): Arduino millis() when mouse exited
            dwell_time (float): Dwell time in seconds
            event (str): Event type ('AUTO' or 'MANUAL')
            
        Returns:
            bool: True if successful, False otherwise
        """
        try:
            self.update_file_path()
            
            # Create data dictionary
            data = {
                'Trial': [trial],
                'Position': [position],
                'DwellTime(s)': [dwell_time],
                'Door Event': [event],
                'Timestamp': [datetime.now().strftime("%Y-%m-%d %H:%M:%S")],                
                'EntryTime': [entry_time],
                'ExitTime': [exit_time]
            }
            
            df = pd.DataFrame(data)
            
            # Append to existing file or create new
            if self.current_file.exists():
                # Append mode
                existing_df = pd.read_excel(self.current_file)
                combined_df = pd.concat([existing_df, df], ignore_index=True)
                combined_df.to_excel(self.current_file, index=False)
            else:
                # Create new file
                df.to_excel(self.current_file, index=False)
            
            return True
            
        except Exception as e:
            print(f"ERROR in log_data: {e}")
            return False
    
    def parse_data_packet(self, line):
        """
        Parse DATA CSV packet from Arduino.
        
        Expected format: DATA,Trial,Position,EntryTime,ExitTime,DwellTime,Event
        Example: DATA,1,5,12543,18865,6.32,AUTO
        
        Args:
            line (str): Raw DATA packet line
            
        Returns:
            bool: True if successfully logged, False otherwise
        """
        try:
            parts = line.split(',')
            if len(parts) == 7 and parts[0] == "DATA":
                trial = int(parts[1])
                position = int(parts[2])
                entry_time = int(parts[3])
                exit_time = int(parts[4])
                dwell_time = float(parts[5])
                event = parts[6].strip()
                
                return self.log_data(trial, position, entry_time, exit_time, 
                                   dwell_time, event)
            else:
                print(f"Invalid DATA packet format: {line}")
                return False
                
        except Exception as e:
            print(f"ERROR parsing DATA packet: {e}")
            return False
    
    def get_data_folder_path(self):
        """
        Get absolute path to data folder.
        
        Returns:
            str: Absolute path to data folder
        """
        return str(self.data_folder.absolute())
    
    def file_exists(self):
        """
        Check if current date's file exists.
        
        Returns:
            bool: True if file exists, False otherwise
        """
        self.update_file_path()
        return self.current_file.exists()
    
    def get_trial_count(self):
        """
        Get number of trials in current file.
        
        Returns:
            int: Number of trials, or 0 if file doesn't exist
        """
        self.update_file_path()
        if self.current_file.exists():
            try:
                df = pd.read_excel(self.current_file)
                return len(df)
            except:
                return 0
        return 0
