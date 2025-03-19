# BioAmp-EXG-Pill: Streaming ECG & EMG Data to OpenViBE via LSL

## Introduction
This guide provides step-by-step instructions to set up and stream ECG and EMG signals from the BioAmp-EXG-Pill to OpenViBE using Lab Streaming Layer (LSL). The steps include uploading firmware to an Arduino, setting up hardware connections, running Python scripts, and configuring OpenViBE for real-time signal processing.

## Requirements
### Hardware
- [BioAmp-EXG-Pill](https://github.com/upsidedownlabs/BioAmp-EXG-Pill)
- Arduino-compatible board
- Electrodes for ECG/EMG data acquisition
- USB cable for Arduino-PC connection

### Software
- Arduino IDE
- Python 3.x with required packages (`numpy`, `pylsl`, `serial`)
- OpenViBE

## Step 1: Uploading Firmware to Arduino
1. Open **Arduino IDE**.
2. Connect your Arduino board via USB.
3. Open the corresponding firmware:
   - For ECG: `ecg_ecgpill.ino`
   - For EMG: `emg_exgpill.ino`
4. Select the correct **Board** and **Port**.
5. Click **Upload** to flash the code onto the board.

## Step 2: Connecting the Electrodes
- Attach the electrodes according to the [BioAmp-EXG-Pill documentation](https://github.com/upsidedownlabs/BioAmp-EXG-Pill/tree/main):
  - **ECG**: Place electrodes on the left and right arms with a ground electrode.
  - **EMG**: Place electrodes on the target muscle and a reference electrode on a nearby bone.

## Step 3: Running the Python Streaming Script
After uploading the firmware and connecting the electrodes, execute the appropriate Python script to stream data via LSL.

1. Open a terminal or command prompt.
2. Navigate to the script directory.
3. Run the script for ECG:
   ```bash
   python ecg_to_openvibe.py --port COM5 --baud 115200 --channels 6 --rate 500
   ```
4. Run the script for EMG:
   ```bash
   python emg_to_openibe.py --port COM5 --baud 115200 --channels 6 --rate 1000
   ```
5. If successful, you should see messages confirming the LSL stream creation.

## Step 4: Setting Up OpenViBE for Real-Time Data Processing
### Creating the Designer Blocks
1. Open **OpenViBE Designer**.
2. Create a new **Scenario**.
3. Add the following blocks:
   - **Acquisition Client** to receive data from the LSL stream.
   - **Signal Display** to visualize raw input signals.
   - **Temporal Filter** to process signals (e.g., removing noise).
   - **Signal Average** to compute average values over time.
   - **Signal Display** (second instance) to visualize processed signals.
4. Connect the blocks accordingly as shown below:


![Designer Block Setup][image](https://github.com/user-attachments/assets/06e3c997-19b8-4edc-9b26-37c1f1a61cf2)

5. Run the scenario and observe the real-time data visualization.

## Step 5: Configuring OpenViBE Acquisition Server
1. Open **OpenViBE Acquisition Server**.
2. Select **LabStreamingLayer (LSL)** as the driver.
3. Set the **Connection Port** to `9999`.
4. Adjust **Sample Count per Sent Block** to `8`.
5. Ensure **Drift Correction** is set to `Force` for accurate synchronization.
6. Set **Drift Tolerance (ms)** to `2`.
7. Click **Apply** and **Connect** to start data acquisition.


![OpenViBE Acquisition Server Settings](https://github.com/user-attachments/assets/ee9a5c6d-9c6d-48a8-b250-416fb5018301)

## Result
After following these steps, you should see real-time ECG/EMG signals streaming in OpenViBE with proper drift correction. Below is an example result from the OpenViBE visualization:
![Result][image](https://github.com/user-attachments/assets/13827a6a-84c4-401b-a38e-96815c3795ab)



![image](https://github.com/user-attachments/assets/535a3b42-86c6-4f72-930c-755395b4faac)


## other
![Server](https://github.com/user-attachments/assets/3e3c44be-b908-4f07-b024-b6bf525af8b0)





## Conclusion
This guide covers the setup for acquiring, streaming, and visualizing ECG/EMG data using BioAmp-EXG-Pill with OpenViBE via LSL. The same approach can be applied to other bio-signal monitoring tasks with appropriate adjustments.
## Contribution

Special thanks to VIKAS SHARMA 
