ARMGNU ?= aarch64-linux-gnu

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMCFLAGS = -Iinclude 

BUILD_DIR = build
SRC_DIR = src

all: kernel8.img

debug: CFLAGS += -g
debug: kernel8.img

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(CFLAGS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMCFLAGS) -MMD -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	mkdir -p $(@D)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

clean:
	rm -rf $(BUILD_DIR) *.img

run-arm: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

run: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio
	
debug-qemu: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -S -s

debug-gdb:
	gdb-multiarch build/kernel8.elf

run-on-board:
	sudo screen /dev/ttyUSB0 115200