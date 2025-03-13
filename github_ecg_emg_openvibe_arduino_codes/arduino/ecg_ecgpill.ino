// ECG Filter - BioAmp EXG Pill - Multi-Channel Version
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

#define SAMPLE_RATE 500     // Sample rate in Hz (appropriate for ECG)
#define BAUD_RATE 115200    // Serial baud rate
#define GAIN_FACTOR 3.0     // Amplification factor for ECG signal
#define DC_OFFSET 512       // Midpoint of Arduino ADC (0-1023)
#define MOVING_AVG_SIZE 3   // Size of moving average filter for noise reduction
#define NUM_CHANNELS 6      // Number of ECG channels

// Define input pins for each channel
const int inputPins[NUM_CHANNELS] = {A0, A1, A2, A3, A4, A5};

// Buffer for moving average filter for each channel
float movingAvgBuffer[NUM_CHANNELS][MOVING_AVG_SIZE];
int bufferIndex[NUM_CHANNELS] = {0};

// Variables for baseline correction for each channel
float baseline[NUM_CHANNELS] = {0};
const float BASELINE_ALPHA = 0.005; // Medium-adapting baseline for ECG

// Filter state variables for each channel
float z1_1[NUM_CHANNELS] = {0}, z2_1[NUM_CHANNELS] = {0};
float z1_2[NUM_CHANNELS] = {0}, z2_2[NUM_CHANNELS] = {0};
float z1_3[NUM_CHANNELS] = {0}, z2_3[NUM_CHANNELS] = {0};
float z1_4[NUM_CHANNELS] = {0}, z2_4[NUM_CHANNELS] = {0};

// Variables for R-peak detection
float threshold[NUM_CHANNELS] = {0};
unsigned long lastPeakTime[NUM_CHANNELS] = {0};
const unsigned long REFRACTORY_PERIOD = 200; // Minimum time between R-peaks in ms
float heartRate[NUM_CHANNELS] = {0};

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
	
	// Initialize thresholds for R-peak detection
	for (int ch = 0; ch < NUM_CHANNELS; ch++) {
		threshold[ch] = 100; // Initial threshold, will adapt
	}
	
	// Print header
	Serial.println("Multi-Channel ECG Recording Started");
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
			
			// Update baseline with medium-adapting filter
			baseline[ch] = baseline[ch] * (1 - BASELINE_ALPHA) + raw_value * BASELINE_ALPHA;
			
			// Remove DC offset and baseline drift
			float centered_value = raw_value - baseline[ch];
			
			// Apply moving average filter to reduce noise
			float filtered_value = applyMovingAverage(centered_value, ch);
			
			// Apply ECG bandpass filter
			float ecg_signal = ECGFilter(filtered_value, ch);
			
			// Apply gain to amplify the signal
			float amplified_signal = ecg_signal * GAIN_FACTOR;
			
			// Detect R-peaks and calculate heart rate
			detectRPeak(amplified_signal, ch);
			
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
	
	// Every second, print heart rate information
	static unsigned long lastHeartRateTime = 0;
	if (millis() - lastHeartRateTime > 1000) {
		lastHeartRateTime = millis();
		
		// Print heart rate for each channel
		Serial.print("Heart Rate (BPM): ");
		for (int ch = 0; ch < NUM_CHANNELS; ch++) {
			Serial.print("CH");
			Serial.print(ch+1);
			Serial.print(": ");
			Serial.print(heartRate[ch], 1);
			if (ch < NUM_CHANNELS - 1) {
				Serial.print(", ");
			}
		}
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

// Detect R-peaks and calculate heart rate
void detectRPeak(float signal, int channel) {
	// Check if signal exceeds threshold and we're not in refractory period
	unsigned long currentTime = millis();
	if (signal > threshold[channel] && (currentTime - lastPeakTime[channel]) > REFRACTORY_PERIOD) {
		// R-peak detected
		lastPeakTime[channel] = currentTime;
		
		// Calculate heart rate based on time between peaks
		static unsigned long previousPeakTime[6] = {0};
		if (previousPeakTime[channel] > 0) {
			// Calculate instantaneous heart rate
			float timeBetweenBeats = (currentTime - previousPeakTime[channel]) / 1000.0; // in seconds
			float instantRate = 60.0 / timeBetweenBeats; // convert to BPM
			
			// Apply smoothing to heart rate
			heartRate[channel] = 0.7 * heartRate[channel] + 0.3 * instantRate;
			
			// Adapt threshold based on signal amplitude
			threshold[channel] = 0.7 * threshold[channel] + 0.3 * (signal * 0.6);
		}
		previousPeakTime[channel] = currentTime;
	}
	
	// Gradually decrease threshold over time to adapt to changing signal amplitude
	if (currentTime - lastPeakTime[channel] > 2000) {
		threshold[channel] *= 0.95;
		if (threshold[channel] < 50) threshold[channel] = 50; // Minimum threshold
	}
}

// Band-Pass Butterworth IIR digital filter for ECG
// Sampling rate: 500.0 Hz, frequency: [0.5, 45.0] Hz.
// Filter is order 4, implemented as second-order sections (biquads).
float ECGFilter(float input, int channel) {
	float output = input;
	{
		float x = output - -1.97779782*z1_1[channel] - 0.97803468*z2_1[channel];
		output = 0.00006157*x + 0.00012315*z1_1[channel] + 0.00006157*z2_1[channel];
		z2_1[channel] = z1_1[channel];
		z1_1[channel] = x;
	}
	{
		float x = output - -1.36529686*z1_2[channel] - 0.68610624*z2_2[channel];
		output = 1.00000000*x + 2.00000000*z1_2[channel] + 1.00000000*z2_2[channel];
		z2_2[channel] = z1_2[channel];
		z1_2[channel] = x;
	}
	{
		float x = output - -1.96888792*z1_3[channel] - 0.97095784*z2_3[channel];
		output = 1.00000000*x + -2.00000000*z1_3[channel] + 1.00000000*z2_3[channel];
		z2_3[channel] = z1_3[channel];
		z1_3[channel] = x;
	}
	{
		float x = output - -1.99518105*z1_4[channel] - 0.99518637*z2_4[channel];
		output = 1.00000000*x + -2.00000000*z1_4[channel] + 1.00000000*z2_4[channel];
		z2_4[channel] = z1_4[channel];
		z1_4[channel] = x;
	}
	return output;
}