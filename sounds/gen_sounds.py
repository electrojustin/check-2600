import wave

waveform_data = []
# Waveform data is stored as - and _ to represent full volume and 0 volume respectively.
# Each character represents a "sample". The AUDFx registers really represent the sampling frequency, the AUDCx registers correspond to a waveform.
with open('waveforms.txt', 'r') as waveform_file:
	waveform_ascii_lines = waveform_file.readlines()[1:]
	for ascii_waveform in waveform_ascii_lines:
		waveform = []
		for character in ascii_waveform:
			if character == '-':
				waveform.append(128)
			elif character == '_':
				waveform.append(0)
		waveform_data.append(waveform)

for i in range(0, 32):
	freq = int(30000/(i+1)) # Approximation used for filename
	for j in range(0, len(waveform_data)):
		# Some sound players might have trouble with weird sampling frequencies, so we just upsample everything to the standard Nyquist frequency
		upsampled_waveform = []
		index = 0
		period = float(i+1) / 30000
		time = 0
		sampling_period = 1.0 / 44100.0
		num_repeats = 3 # Number of times to repeat the waveform. Chosen experimentally to create the least audio jank in Pulseaudio
		for k in range(0, num_repeats*int(len(waveform_data[j])*period/sampling_period)):
			upsampled_waveform.append(waveform_data[j][int(time / period) % len(waveform_data[j])])
			time += sampling_period
		upsampled_waveform = bytearray(upsampled_waveform)

		filename = str(freq) + 'hz_waveform' + str(j) + '.wav'
		file = wave.open(filename, 'wb')
		file.setnchannels(1)
		file.setsampwidth(1)
		file.setframerate(44100)
		file.writeframes(upsampled_waveform)
		file.close()
