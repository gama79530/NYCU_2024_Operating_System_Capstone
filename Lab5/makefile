exec_uploader := python3 ../uploader.py
kernel_path ?= ./kernel/kernel8.img
port ?= /dev/ttyUSB0
timeout ?= 
automatic ?=

cmd_args := $(kernel_path) $(port)
ifneq ($(timeout),)
	cmd_args += --timeout $(timeout)
endif

ifneq ($(automatic), )
	cmd_args += --automatic
endif

all:
	sudo chmod 666 $(port)
	$(exec_uploader) $(cmd_args)

help:
	$(exec_uploader) --help

run-on-board:
	sudo screen /dev/ttyUSB0 115200
	