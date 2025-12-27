ARMGNU ?= aarch64-linux-gnu

OBJCOPY = $(ARMGNU)-objcopy

BUILD_DIR = build
INC_DIR = include

ASMFLAGS = -I$(INC_DIR)

.PHONY: deploy

user.img: linker.ld main.S
	mkdir -p $(BUILD_DIR)
	$(ARMGNU)-gcc $(ASMFLAGS) -c main.S -o $(BUILD_DIR)/main.o
	$(ARMGNU)-ld -T linker.ld -o $(BUILD_DIR)/user.elf $(BUILD_DIR)/main.o
	$(OBJCOPY) $(BUILD_DIR)/user.elf -O binary $(BUILD_DIR)/user.img

deploy: $(BUILD_DIR)/user.img
	cp $(BUILD_DIR)/user.img ../../KernelLoader/initramfs