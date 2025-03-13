import serial
import time
import numpy as np
from pylsl import StreamInfo, StreamOutlet
import argparse

# Default settings
DEFAULT_PORT = 'COM5'
DEFAULT_BAUD = 115200
DEFAULT_CHANNELS = 6
DEFAULT_SAMPLE_RATE = 500  # Match the sample rate in ecg_emgpill.ino (500Hz for ECG)

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Stream Arduino ECG data to OpenViBE via LSL')
    parser.add_argument('--port', default=DEFAULT_PORT, help=f'Serial port (default: {DEFAULT_PORT})')
    parser.add_argument('--baud', type=int, default=DEFAULT_BAUD, help=f'Baud rate (default: {DEFAULT_BAUD})')
    parser.add_argument('--channels', type=int, default=DEFAULT_CHANNELS, help=f'Number of ECG channels (default: {DEFAULT_CHANNELS})')
    parser.add_argument('--rate', type=int, default=DEFAULT_SAMPLE_RATE, help=f'Sample rate (default: {DEFAULT_SAMPLE_RATE})')
    args = parser.parse_args()
    
    try:
        # Open serial connection
        print(f"Connecting to {args.port} at {args.baud} baud...")
        ser = serial.Serial(args.port, args.baud, timeout=1)
        print("Connected!")
        
        # Create LSL stream info
        print("Creating LSL stream...")
        info = StreamInfo(
            name='BioAmpEXG_ECG',
            type='ECG',
            channel_count=args.channels,
            nominal_srate=args.rate,
            channel_format='float32',
            source_id='bioamp_ecg_pill'
        )
        
        # Add channel information
        channels = info.desc().append_child("channels")
        for c in range(args.channels):
            channels.append_child("channel") \
                .append_child_value("label", f"ECG{c+1}") \
                .append_child_value("unit", "microvolts") \
                .append_child_value("type", "ECG")
        
        # Create outlet
        outlet = StreamOutlet(info)
        print(f"LSL stream '{info.name()}' created with {args.channels} channel(s) at {args.rate} Hz")
        print("Now sending ECG data to OpenViBE. Press Ctrl+C to stop.")
        
        # Clear any initial data
        ser.reset_input_buffer()
        time.sleep(1)
        
        # For timing
        last_sample_time = time.time()
        sample_interval = 1.0 / args.rate
        
        # For heart rate tracking
        heart_rates = [0] * args.channels
        last_hr_update = time.time()
        
        # Main loop
        while True:
            # Read data from serial port
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8').strip()
                    
                    # Check if this is a heart rate line
                    if line.startswith("Heart Rate"):
                        # Parse heart rates
                        parts = line.replace("Heart Rate (BPM): ", "").split(", ")
                        for i, part in enumerate(parts):
                            if i < args.channels:
                                try:
                                    # Extract the numeric part (e.g., "CH1: 72.5" -> 72.5)
                                    hr_value = float(part.split(": ")[1])
                                    heart_rates[i] = hr_value
                                except (IndexError, ValueError):
                                    pass
                        
                        # Update last heart rate time
                        last_hr_update = time.time()
                        
                        # Print heart rates
                        print(f"Heart Rates: {heart_rates}")
                        
                    # Process ECG data line (comma-separated values)
                    elif "," in line and not line.startswith("Multi-Channel"):
                        values = line.split(',')
                        if len(values) == args.channels:  # Ensure we have the expected number of channels
                            sample = [float(v) for v in values]
                            
                            # Send sample to LSL
                            outlet.push_sample(sample)
                            
                            # Periodically print data
                            current_time = time.time()
                            if current_time - last_sample_time > 1.0:
                                print(f"Sending ECG data: {sample}")
                                
                                # If we haven't received a heart rate update recently, print the last known values
                                if current_time - last_hr_update > 2.0 and any(hr > 0 for hr in heart_rates):
                                    print(f"Heart Rates: {heart_rates}")
                                
                                last_sample_time = current_time
                                
                except ValueError as e:
                    # Skip lines that can't be parsed
                    pass
                except Exception as e:
                    print(f"Error processing line: {e}")
            else:
                # If no data, sleep a bit to avoid busy waiting
                time.sleep(0.001)
                
    except KeyboardInterrupt:
        print("\nStopping...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Clean up
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial connection closed")
            
if __name__ == "__main__":
    main()