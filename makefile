.PHONY: all run upload lab1 lab2 lab3 lab4 lab5 lab6 lab7 lab8

all: run

SERIAL_PORT ?= /dev/ttyUSB0
BAUD_RATE ?= 115200
KERNEL ?= ./Lab2/c/bin/kernel8.img
UPLOADER_FLAGS ?=

upload:
	python3 KernelUploader.py --kernel $(KERNEL) --port $(SERIAL_PORT) --baud $(BAUD_RATE) $(UPLOADER_FLAGS)

lab1:
	$(MAKE) upload KERNEL=./Lab1/c/bin/kernel8.img

lab2:
	$(MAKE) upload KERNEL=./Lab2/c/bin/kernel8.img

lab3:
	$(MAKE) upload KERNEL=./Lab3/c/bin/kernel8.img

lab4:
	$(MAKE) upload KERNEL=./Lab4/c/bin/kernel8.img

lab5:
	$(MAKE) upload KERNEL=./Lab5/c/bin/kernel8.img

lab6:
	$(MAKE) upload KERNEL=./Lab6/c/bin/kernel8.img

lab7:
	$(MAKE) upload KERNEL=./Lab7/c/bin/kernel8.img

lab8:
	$(MAKE) upload KERNEL=./Lab8/c/bin/kernel8.img

run:
	sudo screen $(SERIAL_PORT) $(BAUD_RATE)
