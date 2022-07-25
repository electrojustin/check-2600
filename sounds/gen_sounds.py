import wave

waveform_data = []
with open('waveforms.txt', 'r') as waveform_file:
	waveform_ascii_lines = waveform_file.readlines()[1:]
	for ascii_waveform in waveform_ascii_lines:
		waveform = []
		for character in ascii_waveform:
			if character == '-':
				waveform.append(1)
			elif character == '_':
				waveform.append(0)
		waveform_data.append(waveform)

for i in range(0, 32):
	freq = int(30000/(i+1))
	for j in range(0, len(waveform_data)):
		upsampled_waveform = []
		index = 0
		period = float(i+1)/30000
		time = 0
		duration = 1.0 / 44100.0
		for k in range(0, int(3*len(waveform_data[j])*period/duration)):
			upsampled_waveform.append(waveform_data[j][int(time / period) % len(waveform_data[j])])
			time += duration
		upsampled_waveform = bytearray(upsampled_waveform)
		filename = str(freq) + 'hz_waveform' + str(j) + '.wav'
		file = wave.open(filename, 'wb')
		file.setnchannels(1)
		file.setsampwidth(1)
		file.setframerate(44100)
		file.writeframes(upsampled_waveform)
		file.close()
