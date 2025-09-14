ARMGNU ?= aarch64-linux-gnu

CC = $(ARMGNU)-gcc
LD = $(ARMGNU)-ld
OBJCOPY = $(ARMGNU)-objcopy

BUILD_DIR = build
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only -I$(INC_DIR)
ASMFLAGS = -I$(INC_DIR)
LDFLAGS =
DEBUG_FLAGS = -g

CPIO_ARG = -initrd $(BIN_DIR)/initramfs.cpio
DTB_ARG = -dtb $(BIN_DIR)/bcm2710-rpi-3-b.dtb

.PHONY: all symbol-debug clean run qemu qemu-asm debug-qemu gdb pty package

all: $(BIN_DIR)/kernel8.img

symbol-debug:
	@$(MAKE) -s clean
	@$(MAKE) all CFLAGS="$(CFLAGS) $(DEBUG_FLAGS)" ASMFLAGS="$(ASMFLAGS) $(DEBUG_FLAGS)" LDFLAGS="$(LDFLAGS) $(DEBUG_FLAGS)"


$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	@mkdir -p $(@D)
	$(CC) $(ASMFLAGS) -MMD -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

$(BIN_DIR)/kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES) -Map=$(BUILD_DIR)/kernel8.map
	$(OBJCOPY) $(BUILD_DIR)/kernel8.elf -O binary $@
	
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/kernel8.img

SERIAL_PORT = /dev/ttyUSB0
BAUD_RATE = 115200

run:
	sudo screen $(SERIAL_PORT) $(BAUD_RATE)

QEMU ?= qemu-system-aarch64
QEMU_FLAGS_QEMU = -M raspi3b -kernel $(BIN_DIR)/kernel8.img -display none -serial null
QEMU_FLAGS_QEMU_ASM = -M raspi3b -kernel $(BIN_DIR)/kernel8.img -display none -d in_asm
QEMU_FLAGS_DEBUG_QEMU = -M raspi3b -kernel $(BIN_DIR)/kernel8.img -serial null -serial stdio -display none -S -s

qemu:
	@$(QEMU) $(QEMU_FLAGS_QEMU) -serial stdio $(DTB_ARG) $(CPIO_ARG)

qemu-asm:
	@$(QEMU) $(QEMU_FLAGS_QEMU_ASM)

debug-qemu:
	$(QEMU) $(QEMU_FLAGS_DEBUG_QEMU) $(DTB_ARG) $(CPIO_ARG)

gdb:
	gdb-multiarch $(BUILD_DIR)/kernel8.elf

pty:
	@$(QEMU) $(QEMU_FLAGS_QEMU) -serial pty -serial stdio $(DTB_ARG) $(CPIO_ARG)

package:
	@rm -rf bin/initramfs.cpio
	cd initramfs; find . | cpio -o -H newc > ../bin/initramfs.cpio
	