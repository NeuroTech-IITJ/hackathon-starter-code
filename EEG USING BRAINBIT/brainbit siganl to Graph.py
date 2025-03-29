from neurosdk.scanner import Scanner
from neurosdk.cmn_types import *

import matplotlib.pyplot as plt
import matplotlib.animation as animation
from time import sleep

# EEG Buffer Setup
channel_data = {"O1": [], "O2": [], "T3": [], "T4": []}
MAX_BUFFER_SIZE = 1000
scale_factor = 500  # Optimized scale factor

# EEG Data Receiver
def on_signal_received(sensor, data):
    try:
        for item in data:
            for ch in channel_data.keys():
                raw_value = getattr(item, ch, None)
                if raw_value is not None:
                    scaled_value = raw_value * scale_factor
                    channel_data[ch].append(scaled_value)

        for ch in channel_data:
            if len(channel_data[ch]) > MAX_BUFFER_SIZE:
                channel_data[ch] = channel_data[ch][-MAX_BUFFER_SIZE:]

        # Enhanced debugging output
        if len(channel_data["O1"]) % 50 == 0:
            print("\n--- Last 10 EEG Samples (Scaled) ---")
            for ch in channel_data:
                print(f"{ch}: {channel_data[ch][-10:]}")
            print("------------------------")

    except Exception as e:
        print("Signal data error:", e)

# Plot Setup
fig, axs = plt.subplots(4, 1, figsize=(10, 8), sharex=True)
fig.suptitle("Live EEG Signals - O1, O2, T3, T4 (Scaled)")

lines = {}
for idx, ch in enumerate(channel_data):
    axs[idx].set_ylim(-200, 200)  # Optimized range
    axs[idx].set_xlim(0, 250)
    axs[idx].set_ylabel(f"{ch} (a.u.)")
    axs[idx].grid(True)
    (line,) = axs[idx].plot([], [], label=ch)
    lines[ch] = line

axs[-1].set_xlabel("Sample Index")

def init():
    for line in lines.values():
        line.set_data([], [])
    return list(lines.values())

def update(frame):
    for ch, line in lines.items():
        data = channel_data[ch][-250:]
        x = list(range(len(data)))
        line.set_data(x, data)
    return list(lines.values())

ani = animation.FuncAnimation(fig, update, init_func=init, interval=100, blit=True)

# Main Execution
try:
    print("Searching for sensors...")
    scanner = Scanner([SensorFamily.LEBrainBit, SensorFamily.LEBrainBitBlack])
    scanner.start()
    sleep(5)
    scanner.stop()

    sensors_info = scanner.sensors()
    if not sensors_info:
        print("No sensors found.")
        exit()

    sensor = scanner.create_sensor(sensors_info[0])
    print(f"Connected to: {sensor.name}")

    if sensor.is_supported_parameter(SensorParameter.Gain):
        sensor.gain = SensorGain.Gain12
        print(f"Gain set to: {sensor.gain}")

    sensor.signalDataReceived = on_signal_received
    if sensor.is_supported_command(SensorCommand.StartSignal):
        sensor.exec_command(SensorCommand.StartSignal)
        print("Receiving EEG data...")

    plt.tight_layout()
    plt.show()

    if sensor.is_supported_command(SensorCommand.StopSignal):
        sensor.exec_command(SensorCommand.StopSignal)

    sensor.disconnect()
    del sensor
    del scanner
    print("Sensor disconnected.")

except Exception as e:
    print(f"Error: {e}")
