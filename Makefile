FQBN ?= SiliconLabs:silabs:nano_matter:protocol_stack=none
TX_SKETCH = TallyBlinkControlEcho.ino
RX_SKETCH = TallyBlinkControlReceiver
LIBRARY = libraries/BMDSDIControl
PORT ?= $(shell arduino-cli board list | awk '/Nano Matter|usbmodem/ {print $$1; exit}')

.PHONY: compile compile-tx compile-rx upload upload-tx upload-rx monitor boards clean

compile: compile-tx

compile-tx:
	arduino-cli compile --fqbn $(FQBN) --library $(LIBRARY) $(TX_SKETCH)

compile-rx:
	arduino-cli compile --fqbn $(FQBN) --library $(LIBRARY) $(RX_SKETCH)

upload: upload-tx

upload-tx: compile-tx
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) $(TX_SKETCH)

upload-rx: compile-rx
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) $(RX_SKETCH)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=115200

boards:
	arduino-cli board list

clean:
	rm -rf build