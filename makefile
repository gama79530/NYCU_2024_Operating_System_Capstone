all: uploader

.PHONY: uploader run

uploader:
	docker compose run --rm operating_system_capstone -c "python3 KernelUploader.py"

SERIAL_PORT = /dev/ttyUSB0
BAUD_RATE = 115200

run:
	sudo screen $(SERIAL_PORT) $(BAUD_RATE)
