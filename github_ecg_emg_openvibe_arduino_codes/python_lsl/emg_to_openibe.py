import serial
import time
import numpy as np
from pylsl import StreamInfo, StreamOutlet
import argparse

# Default settings
DEFAULT_PORT = 'COM5'
DEFAULT_BAUD = 115200
DEFAULT_CHANNELS = 6
DEFAULT_SAMPLE_RATE = 1000  # Match the sample rate in emg.ino (1000Hz for EMG)

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Stream Arduino EMG data to OpenViBE via LSL')
    parser.add_argument('--port', default=DEFAULT_PORT, help=f'Serial port (default: {DEFAULT_PORT})')
    parser.add_argument('--baud', type=int, default=DEFAULT_BAUD, help=f'Baud rate (default: {DEFAULT_BAUD})')
    parser.add_argument('--channels', type=int, default=DEFAULT_CHANNELS, help=f'Number of EMG channels (default: {DEFAULT_CHANNELS})')
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
            name='BioAmpEXG_EMG',
            type='EMG',
            channel_count=args.channels,
            nominal_srate=args.rate,
            channel_format='float32',
            source_id='bioamp_emg_pill'
        )
        
        # Add channel information
        channels = info.desc().append_child("channels")
        for c in range(args.channels):
            channels.append_child("channel") \
                .append_child_value("label", f"EMG{c+1}") \
                .append_child_value("unit", "microvolts") \
                .append_child_value("type", "EMG")
        
        # Create outlet
        outlet = StreamOutlet(info)
        print(f"LSL stream '{info.name()}' created with {args.channels} channel(s) at {args.rate} Hz")
        print("Now sending EMG data to OpenViBE. Press Ctrl+C to stop.")
        
        # Clear any initial data
        ser.reset_input_buffer()
        time.sleep(1)
        
        # For timing
        last_sample_time = time.time()
        sample_interval = 1.0 / args.rate
        
        # For signal quality monitoring
        signal_monitor_interval = 5.0  # Check signal quality every 5 seconds
        last_monitor_time = time.time()
        signal_buffer = []
        
        # Main loop
        while True:
            # Read data from serial port
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8').strip()
                    if line:
                        values = line.split(',')
                        sample = [float(v) for v in values]
                        
                        # Store sample for signal quality monitoring
                        signal_buffer.append(sample)
                        if len(signal_buffer) > args.rate * signal_monitor_interval:
                            signal_buffer.pop(0)
                        
                        # Send sample to LSL
                        outlet.push_sample(sample)
                        
                        # Periodically print data
                        current_time = time.time()
                        if current_time - last_sample_time > 1.0:
                            print(f"Sending data: {sample}")
                            last_sample_time = current_time
                            
                        # Periodically check signal quality
                        if current_time - last_monitor_time > signal_monitor_interval and signal_buffer:
                            last_monitor_time = current_time
                            signal_array = np.array(signal_buffer)
                            rms_values = np.sqrt(np.mean(np.square(signal_array), axis=0))
                            print("\nSignal Quality Check:")
                            for ch in range(len(rms_values)):
                                quality = "Good" if 5 < rms_values[ch] < 500 else "Check electrodes"
                                print(f"  Channel EMG{ch+1}: RMS = {rms_values[ch]:.2f}µV - {quality}")
                            print("")
                            
                except ValueError:
                    pass
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