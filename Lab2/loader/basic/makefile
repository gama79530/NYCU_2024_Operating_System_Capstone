ARMGNU ?= aarch64-linux-gnu

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMCFLAGS = -Iinclude 

BUILD_DIR = build
SRC_DIR = src

all: bootloader.img

debug: CFLAGS += -g
debug: bootloader.img

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

bootloader.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	mkdir -p $(@D)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/bootloader.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/bootloader.elf -O binary bootloader.img

clean:
	rm -rf $(BUILD_DIR) *.img

run: 
	qemu-system-aarch64 -M raspi3b -device loader,file=bootloader.img,addr=0x60000,cpu-num=0 -display none -serial null -serial stdio
	
pty: 
	qemu-system-aarch64 -M raspi3b -device loader,file=bootloader.img,addr=0x60000,cpu-num=0 -display none -serial null -serial pty

debug-qemu: 
	qemu-system-aarch64 -M raspi3b -device loader,file=bootloader.img,addr=0x60000,cpu-num=0 -serial null -serial stdio -display none -S -s

debug-gdb:
	gdb-multiarch build/bootloader.elf
