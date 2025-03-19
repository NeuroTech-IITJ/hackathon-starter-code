// EMG Filter - BioAmp EXG Pill - Multi-Channel Version
// https://github.com/upsidedownlabs/BioAmp-EXG-Pill

// Upside Down Labs invests time and resources providing this open source code,
// please support Upside Down Labs and open-source hardware by purchasing
// products from Upside Down Labs!

// Copyright (c) 2021 Upside Down Labs - contact@upsidedownlabs.tech

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define SAMPLE_RATE 1000    // Sample rate in Hz (increased for EMG)
#define BAUD_RATE 115200    // Serial baud rate
#define GAIN_FACTOR 2.0     // Amplification factor for EMG signal (reduced as EMG signals are stronger)
#define DC_OFFSET 512       // Midpoint of Arduino ADC (0-1023)
#define MOVING_AVG_SIZE 2   // Size of moving average filter (reduced for EMG to preserve transients)
#define NUM_CHANNELS 6      // Number of EMG channels

// Define input pins for each channel
const int inputPins[NUM_CHANNELS] = {A0};

// Buffer for moving average filter for each channel
float movingAvgBuffer[NUM_CHANNELS][MOVING_AVG_SIZE];
int bufferIndex[NUM_CHANNELS] = {0};

// Variables for baseline correction for each channel
float baseline[NUM_CHANNELS] = {0};
const float BASELINE_ALPHA = 0.01; // Faster-adapting baseline for EMG

// Filter state variables for each channel
float z1_1[NUM_CHANNELS] = {0}, z2_1[NUM_CHANNELS] = {0};
float z1_2[NUM_CHANNELS] = {0}, z2_2[NUM_CHANNELS] = {0};
float z1_3[NUM_CHANNELS] = {0}, z2_3[NUM_CHANNELS] = {0};
float z1_4[NUM_CHANNELS] = {0}, z2_4[NUM_CHANNELS] = {0};

void setup() {
	// Serial connection begin
	Serial.begin(BAUD_RATE);
	
	// Initialize moving average buffer for each channel
	for (int ch = 0; ch < NUM_CHANNELS; ch++) {
		for (int i = 0; i < MOVING_AVG_SIZE; i++) {
			movingAvgBuffer[ch][i] = 0;
		}
	}
	
	// Wait a moment for the BioAmp to stabilize
	delay(1000);
	
	// Initialize baseline with first few readings for each channel
	for (int ch = 0; ch < NUM_CHANNELS; ch++) {
		float sum = 0;
		for (int i = 0; i < 100; i++) {
			sum += analogRead(inputPins[ch]);
			delay(5); // Shorter delay to avoid long startup time
		}
		baseline[ch] = sum / 100;
	}
	
	// Print header
	Serial.println("Multi-Channel EMG Recording Started");
}

void loop() {
	// Calculate elapsed time
	static unsigned long past = 0;
	unsigned long present = micros();
	unsigned long interval = present - past;
	past = present;

	// Run timer
	static long timer = 0;
	timer -= interval;

	// Sample at the specified rate
	if (timer < 0) {
		timer += 1000000 / SAMPLE_RATE;
		
		// Process each channel
		for (int ch = 0; ch < NUM_CHANNELS; ch++) {
			// Read raw sensor value
			float raw_value = analogRead(inputPins[ch]);
			
			// Update baseline with faster-adapting filter for EMG
			baseline[ch] = baseline[ch] * (1 - BASELINE_ALPHA) + raw_value * BASELINE_ALPHA;
			
			// Remove DC offset and baseline drift
			float centered_value = raw_value - baseline[ch];
			
			// Apply moving average filter to reduce noise
			float filtered_value = applyMovingAverage(centered_value, ch);
			
			// Apply EMG bandpass filter
			float emg_signal = EMGFilter(filtered_value, ch);
			
			// Apply gain to amplify the signal
			float amplified_signal = emg_signal * GAIN_FACTOR;
			
			// Send the processed signal to serial
			Serial.print(amplified_signal);
			
			// Add comma between values, but not after the last one
			if (ch < NUM_CHANNELS - 1) {
				Serial.print(",");
			}
		}
		
		// End the line after all channels are sent
		Serial.println();
	}
}

// Apply moving average filter to reduce high-frequency noise
float applyMovingAverage(float newValue, int channel) {
	// Add new value to buffer
	movingAvgBuffer[channel][bufferIndex[channel]] = newValue;
	bufferIndex[channel] = (bufferIndex[channel] + 1) % MOVING_AVG_SIZE;
	
	// Calculate average
	float sum = 0;
	for (int i = 0; i < MOVING_AVG_SIZE; i++) {
		sum += movingAvgBuffer[channel][i];
	}
	
	return sum / MOVING_AVG_SIZE;
}

// Band-Pass Butterworth IIR digital filter, generated using filter_gen.py.
// Sampling rate: 1000.0 Hz, frequency: [20.0, 450.0] Hz.
// Filter is order 4, implemented as second-order sections (biquads).
// Reference: 
// https://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.butter.html
// https://courses.ideate.cmu.edu/16-223/f2020/Arduino/FilterDemos/filter_gen.py
float EMGFilter(float input, int channel) {
	float output = input;
	{
		float x = output - -1.72569845*z1_1[channel] - 0.76816491*z2_1[channel];
		output = 0.01667281*x + 0.03334562*z1_1[channel] + 0.01667281*z2_1[channel];
		z2_1[channel] = z1_1[channel];
		z1_1[channel] = x;
	}
	{
		float x = output - -1.85373847*z1_2[channel] - 0.86788663*z2_2[channel];
		output = 1.00000000*x + 2.00000000*z1_2[channel] + 1.00000000*z2_2[channel];
		z2_2[channel] = z1_2[channel];
		z1_2[channel] = x;
	}
	{
		float x = output - -0.41910782*z1_3[channel] - 0.11906657*z2_3[channel];
		output = 1.00000000*x + -2.00000000*z1_3[channel] + 1.00000000*z2_3[channel];
		z2_3[channel] = z1_3[channel];
		z1_3[channel] = x;
	}
	{
		float x = output - -0.59048799*z1_4[channel] - 0.22611322*z2_4[channel];
		output = 1.00000000*x + -2.00000000*z1_4[channel] + 1.00000000*z2_4[channel];
		z2_4[channel] = z1_4[channel];
		z1_4[channel] = x;
	}
	return output;
}
