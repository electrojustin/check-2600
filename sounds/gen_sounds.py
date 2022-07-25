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
		waveform_data.append(bytearray(waveform))

for i in range(0, 32):
	freq = int(30000/(i+1))
	for j in range(0, len(waveform_data)):
		filename = str(freq) + 'hz_waveform' + str(j) + '.wav'
		file = wave.open(filename, 'wb')
		file.setnchannels(1)
		file.setsampwidth(1)
		file.setframerate(freq)
		file.writeframes(waveform_data[j])
		file.close()
