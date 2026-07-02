FQBN ?= SiliconLabs:silabs:nano_matter:protocol_stack=none
TX_SKETCH = TallyBlinkControlEcho.ino
RX_SKETCH = TallyBlinkControlReceiver
LIBRARY = libraries/BMDSDIControl
INCLUDE_DIR = include
BUILD_FLAGS = --build-property compiler.cpp.extra_flags="-I$(CURDIR) -I$(CURDIR)/$(INCLUDE_DIR)"
MONITOR_SCRIPT = scripts/monitor.sh
BAUD ?= 115200

# Default ports — run `make identify-ports` if TX/RX look swapped.
# Decode/TALLY->LoRa on the wrong monitor label means ports are reversed.
TX_PORT ?= /dev/cu.usbmodemE9EB49503
RX_PORT ?= /dev/cu.usbmodem9582F70C3
PORT ?= $(TX_PORT)

.PHONY: compile compile-tx compile-rx upload upload-tx upload-rx upload-both \
        monitor monitor-tx monitor-rx monitor-both stop-monitor-port ports identify-ports boards clean

compile: compile-tx

compile-tx:
	arduino-cli compile --fqbn $(FQBN) --library $(LIBRARY) $(BUILD_FLAGS) $(TX_SKETCH)

compile-rx:
	arduino-cli compile --fqbn $(FQBN) --library $(LIBRARY) $(BUILD_FLAGS) $(RX_SKETCH)

upload: upload-tx

upload-tx: compile-tx
	@echo ">>> Uploading $(TX_SKETCH) to TX_PORT=$(TX_PORT)"
	$(MAKE) stop-monitor-port PORT=$(TX_PORT)
	arduino-cli upload --fqbn $(FQBN) -p $(TX_PORT) $(TX_SKETCH)

upload-rx: compile-rx
	@echo ">>> Uploading $(RX_SKETCH) to RX_PORT=$(RX_PORT)"
	$(MAKE) stop-monitor-port PORT=$(RX_PORT)
	arduino-cli upload --fqbn $(FQBN) -p $(RX_PORT) $(RX_SKETCH)

upload-both: upload-tx upload-rx

# Foreground monitor — run in a separate Terminal tab (auto-reconnects after upload).
monitor: monitor-tx

monitor-tx:
	@$(MONITOR_SCRIPT) TX $(TX_PORT) $(BAUD)

monitor-rx:
	@$(MONITOR_SCRIPT) RX $(RX_PORT) $(BAUD)

monitor-both:
	@$(MONITOR_SCRIPT) TX $(TX_PORT) $(BAUD) & \
	 $(MONITOR_SCRIPT) RX $(RX_PORT) $(BAUD) & \
	 wait

# Release the serial port before programming (called automatically by upload-*).
stop-monitor-port:
	@pkill -f "arduino-cli monitor.*$(PORT)" 2>/dev/null || true
	@sleep 0.5

ports:
	@echo "Configured ports:"
	@echo "  TX_PORT = $(TX_PORT)"
	@echo "  RX_PORT = $(RX_PORT)"
	@echo ""
	@echo "Connected boards:"
	@arduino-cli board list

identify-ports:
	@python3 scripts/identify_ports.py $(TX_PORT) $(RX_PORT)

boards:
	arduino-cli board list

clean:
	rm -rf build .monitor