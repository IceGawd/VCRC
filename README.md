# VCRC (Voice Controlled Robotic Car)

This project is a speech-controlled robot that listens for voice commands and controls motors accordingly. It uses **PocketSphinx** for speech recognition, **PortAudio** for audio processing, and **WiringPi** for GPIO control on a Raspberry Pi.

## Features
- **Speech Recognition**: Listens for voice commands using PocketSphinx.
- **Command Queue**: Stores commands and executes them with a delay.
- **Motor Control**: Uses GPIO pins to control forward, backward, left, and right movement.
- **Multithreading**: Runs speech recognition in a separate thread.
- **Custom Activation Phrase**: Responds only when activation words (e.g., "c r c") are detected.

## Hardware Requirements
- Raspberry Pi (or any compatible system with GPIO support)
- Motor driver and motors
- Microphone for audio input (both USB or Bluetooth work)

## Software Requirements
- **C Compiler** (e.g., `gcc`)
- **PocketSphinx** (for speech recognition)
- **PortAudio** (for audio streaming)
- **WiringPi** (for GPIO control)
- **pthread** (for multithreading)

## Installation & Setup
1. **Install Dependencies**:
	```sh
	sudo apt-get install wiringpi
	sudo apt-get install libasound2-dev portaudio19-dev
	sudo apt-get install pocketsphinx
	```
2. **Pull and Build**:
	```sh
	git clone https://github.com/cmusphinx/pocketsphinx.git
	cd pocketsphinx
	./autogen.sh
	make
	sudo make install
	cd ..
	```
2. **Compile the Program**:
	```sh
	gcc -o voice_recognition voice_recognition.c -lwiringPi -lpthread -lsphinxbase -lpocketsphinx -lportaudio
	```
3. **Run the Program**:
	```sh
	./voice_recognition
	```

## Usage
1. Speak the activation phrase: **"c r c"**
2. Say a command: "forward", "backward", "left", "right", "stop"
3. The robot executes the command after a short delay.
4. Speak a deactivation phrase (such as **"deactivate"**) to put it to sleep.

## File Structure
```
├── voice_recognition.c   # Main program file
├── numbers.c           # Contains number parsing code
├── README.md        # Documentation
```

## Future Improvements
- Improve speech recognition accuracy.
- Add more natural language commands.
- Implement real-time feedback on recognized commands.

## License
This project is open-source under the **MIT License**.