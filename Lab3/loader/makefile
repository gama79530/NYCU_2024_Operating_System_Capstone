ARMGNU ?= aarch64-linux-gnu

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMCFLAGS = -Iinclude 

BUILD_DIR = build
SRC_DIR = src

USER_DIR = user

CPIO = -initrd initramfs.cpio
DTB = -dtb bcm2710-rpi-3-b.dtb

all: kernel8.img user.img ramdisk
	
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

user.img: $(USER_DIR)/src/linker.ld $(USER_DIR)/src/main.S
	mkdir -p $(USER_DIR)/build
	$(ARMGNU)-gcc $(ASMCFLAGS) -c $(USER_DIR)/src/main.S -o $(USER_DIR)/build/main.o
	$(ARMGNU)-ld -T $(USER_DIR)/src/linker.ld -o $(USER_DIR)/build/user.elf $(USER_DIR)/build/main.o
	$(ARMGNU)-objcopy $(USER_DIR)/build/user.elf -O binary rootfs/user.img

ramdisk:
	cd rootfs; find . | cpio -o -H newc > ../initramfs.cpio; cd ..

clean:
	rm -rf $(BUILD_DIR) *.img
	rm -rf initramfs.cpio
	rm -rf $(USER_DIR)/build
	rm -rf rootfs/user.img

run: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio $(DTB) $(CPIO)
	
pty: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial pty $(DTB) $(CPIO)

debug-qemu: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -S -s $(DTB) $(CPIO)

debug-gdb:
	gdb-multiarch build/kernel8.elf
