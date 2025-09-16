all: run

.PHONY: uploader run lab1 lab1cpp

kernel =

lab1: kernel += --kernel ./Lab1/c/bin/kernel8.img
lab1: uploader

lab1cpp: kernel += --kernel ./Lab1/cpp/bin/kernel8.img
lab1cpp: uploader


lab2: kernel += --kernel ./Lab2/c/bin/kernel8.img
lab2: uploader

uploader:
	docker compose run --rm operating_system_capstone -c "python3 KernelUploader.py $(kernel)"

SERIAL_PORT = /dev/ttyUSB0
BAUD_RATE = 115200

run:
	sudo screen $(SERIAL_PORT) $(BAUD_RATE)
