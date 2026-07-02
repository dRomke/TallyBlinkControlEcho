FQBN ?= SiliconLabs:silabs:nano_matter:protocol_stack=none
TX_SKETCH = WirelessANC_TX
RX_SKETCH = WirelessANC_RX
LIBRARY = libraries/BMDSDIControl
INCLUDE_DIR = include
BUILD_FLAGS = --build-property compiler.cpp.extra_flags="-I$(CURDIR) -I$(CURDIR)/$(INCLUDE_DIR)"
MONITOR_SCRIPT = scripts/monitor.sh
BAUD ?= 115200
BUILD_ID = wirelessanc-v1

# Default ports — run `make identify-ports` if TX/RX look swapped.
TX_PORT ?= /dev/cu.usbmodem9582F70C3
RX_PORT ?= /dev/cu.usbmodemE9EB49503
PORT ?= $(TX_PORT)

.PHONY: compile compile-tx compile-rx upload upload-tx upload-rx upload-both \
        monitor monitor-tx monitor-rx monitor-both stop-monitor-port ports identify-ports boards clean docs

compile: compile-tx

compile-tx:
	arduino-cli compile --fqbn $(FQBN) --library $(LIBRARY) $(BUILD_FLAGS) $(TX_SKETCH)

compile-rx:
	arduino-cli compile --fqbn $(FQBN) --library $(LIBRARY) $(BUILD_FLAGS) $(RX_SKETCH)

upload: upload-tx

upload-tx: compile-tx
	@echo ">>> Uploading $(TX_SKETCH) to TX_PORT=$(TX_PORT)"
	$(MAKE) stop-monitor-port PORT=$(TX_PORT)
	@TX_HEX=$$(ls -t $(HOME)/Library/Caches/arduino/sketches/*/WirelessANC_TX.ino.hex 2>/dev/null | head -1); \
		test -n "$$TX_HEX" || (echo "FAIL: TX hex not found — run make compile-tx" && exit 1); \
		python3 scripts/flash_board.py $(TX_PORT) "$$TX_HEX"
	@sleep 3
	@python3 scripts/verify_serial.py $(TX_PORT) "$(BUILD_ID)" 12 || \
		(echo "WARN: TX programmed — reset board and run: make verify-tx" && true)

upload-rx: compile-rx
	@echo ">>> Uploading $(RX_SKETCH) to RX_PORT=$(RX_PORT)"
	$(MAKE) stop-monitor-port PORT=$(RX_PORT)
	@RX_HEX=$$(ls -t $(HOME)/Library/Caches/arduino/sketches/*/WirelessANC_RX.ino.hex 2>/dev/null | head -1); \
		test -n "$$RX_HEX" || (echo "FAIL: RX hex not found — run make compile-rx" && exit 1); \
		python3 scripts/flash_board.py $(RX_PORT) "$$RX_HEX"
	@sleep 3
	@python3 scripts/verify_serial.py $(RX_PORT) "$(BUILD_ID)" 12 || \
		(echo "WARN: RX programmed — reset board and run: make verify-rx" && true)

upload-both: upload-tx upload-rx

monitor: monitor-tx

monitor-tx:
	@$(MONITOR_SCRIPT) TX $(TX_PORT) $(BAUD)

monitor-rx:
	@$(MONITOR_SCRIPT) RX $(RX_PORT) $(BAUD)

monitor-both:
	@$(MONITOR_SCRIPT) TX $(TX_PORT) $(BAUD) & \
	 $(MONITOR_SCRIPT) RX $(RX_PORT) $(BAUD) & \
	 wait

stop-monitor-port:
	@pkill -f "arduino-cli monitor.*$(PORT)" 2>/dev/null || true
	@pkill -f "monitor.py.*$(PORT)" 2>/dev/null || true
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

verify-tx:
	@python3 scripts/verify_serial.py $(TX_PORT) $(BUILD_ID) 12

verify-rx:
	@python3 scripts/verify_serial.py $(RX_PORT) $(BUILD_ID) 12

boards:
	arduino-cli board list

docs:
	@python3 scripts/generate_docs_pdf.py

clean:
	rm -rf build .monitor