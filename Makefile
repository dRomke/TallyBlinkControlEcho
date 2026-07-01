FQBN ?= SiliconLabs:silabs:nano_matter:protocol_stack=none
SKETCH = TallyBlinkControlEcho.ino
LIBRARY = libraries/BMDSDIControl
PORT ?= $(shell arduino-cli board list | awk '/Nano Matter|usbmodem/ {print $$1; exit}')

.PHONY: compile upload monitor boards clean

compile:
	arduino-cli compile --fqbn $(FQBN) --library $(LIBRARY) $(SKETCH)

upload: compile
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) $(SKETCH)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=115200

boards:
	arduino-cli board list

clean:
	rm -rf build