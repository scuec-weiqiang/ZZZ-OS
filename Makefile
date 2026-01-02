#--------------架构---------------#
ARCH ?= riscv64
CROSS_COMPILE ?= riscv64-unknown-elf-
BOARD ?= qemu_virt

DISK = ./disk.img
DISK_DEV = /dev/loop0p1
MOUNT_PATH = ./mount

#--------------输出目录---------------#
BUILD_DIR := build
TARGET := $(BUILD_DIR)/kernel.bin
ELF := $(BUILD_DIR)/kernel.elf

#--------------编译器---------------#
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJDUMP :=$(CROSS_COMPILE)objdump
OBJCOPY := $(CROSS_COMPILE)objcopy

CFLAGS = -g -Wall -fno-builtin -mcmodel=medany 
CFLAGS += -Iinclude 
CFLAGS += -MMD -MP
ASFLAGS := $(CFLAGS)
LDFLAGS := -Tarch/$(ARCH)/link.ld 
LDFLAGS += -nostdlib  # 不链接标准库
LDFLAGS += -nostartfiles  # 不使用标准启动文件（如 crt0.o）
LDFLAGS += -ffreestanding  # 告知编译器这是独立环境（无 OS）
-include arch/$(ARCH)/config.mk

# 目标架构的asm头文件目录（如arch/riscv64/include/asm）
ARCH_ASM_DIR := arch/$(ARCH)/include/asm
# 软链接路径（include/asm）
ASM_LINK := include/asm

$(ASM_LINK):
	-rmdir -p include/asm
	ln -sf ../$(ARCH_ASM_DIR) $(ASM_LINK)

#--------------分层构建---------------#
KBUILD_PATH = tools/kbuild
KBUILD = $(KBUILD_PATH)/kbuild
SRC_ROOT ?=  ./
# Kbuild 输出文件
KBUILD_FILE = objs.mk
# 若不存在 objs.mk，则生成
$(KBUILD_FILE):$(KBUILD)
	@$(KBUILD) $(SRC_ROOT) > $@
$(KBUILD):
	$(MAKE) -C $(KBUILD_PATH)
include $(KBUILD_FILE)
# 生成 .o 文件路径
BUILD_OBJS := $(patsubst %.o, $(BUILD_DIR)/%.o, $(OBJ_Y))


#--------------设备树---------------#
DTC_PATH = tools/dtc
DTC = $(DTC_PATH)/dtc
DTS :=  arch/$(ARCH)/dts/$(BOARD).dts
DTB := $(BUILD_DIR)/$(DTS:.dts=.dtb)
$(DTB):$(DTC)

$(DTC):
	$(MAKE) -C ./tools/dtc

#--------------通用编译---------------#
all: disk $(TARGET) $(DTB) 
	sudo losetup -D
	sudo losetup -Pf --show disk.img
	sudo mount  $(DISK_DEV) $(MOUNT_PATH) && echo "挂载成功!" 
	sudo cp $(TARGET) $(MOUNT_PATH)
	sudo cp $(DTB) $(MOUNT_PATH)
	sudo umount $(MOUNT_PATH)

$(TARGET): $(ELF) 
	$(OBJCOPY) -O binary $< $@
	@echo "$@ is ready"
	
$(ELF): $(BUILD_OBJS) 
	$(CC) $(LDFLAGS) $^ -o $@
	@echo "$@ is ready"

$(BUILD_DIR)/%.o: %.c $(ASM_LINK) $(KBUILD_FILE) 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(dir $<) -c $< -o $@

$(BUILD_DIR)/%.o: %.S $(ASM_LINK) $(KBUILD_FILE)
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -I$(dir $<) -c $< -o $@

$(BUILD_DIR)/%.o: %.s $(ASM_LINK) $(KBUILD_FILE)
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -I$(dir $<) -c $< -o $@
	@echo "汇编：$< → $@"

clean:
	rm -rf $(BUILD_DIR) $(KBUILD_FILE) 
	@echo "clean complete"

.PHONY: all clean

-include $(BUILD_OBJS:.o=.d)



#--------------kbuild工具编译---------------#
kbuild: $(KBUILD)

clean_kbuild:
	$(MAKE) -C $(KBUILD_PATH) clean


#--------------设备树编译---------------#
dtbs: $(DTB)
	sudo mount $(DISK_DEV)  $(MOUNT_PATH) && echo "挂载成功!" 
	sudo cp $(DTB) $(MOUNT_PATH)/
	sudo umount $(MOUNT_PATH)
clean_dtbs:
	rm -f *.dtb
	

$(BUILD_DIR)/%.dtb: %.dts
	mkdir -p $(BUILD_DIR)/arch/$(ARCH)/dts
	$(DTC) -I dts -O dtb -o $@ $< -i .


.PHONY: dtc
dtc:
	$(MAKE) -C tools/dtc 

.PHONY: distclean
distclean:
	@echo "正在清理所有输出文件："
	-rm -rf $(BUILD_DIR)
	-$(MAKE) -C $(DTC_PATH) clean
	-$(MAKE) -C $(KBUILD_PATH) clean
	-rm -f disk.img
	-sudo losetup -D
	-rm -rf $(MOUNT_PATH)
	-rm -rf $(ASM_LINK)
	@echo "清理完成"



#********************************************************************************
.PHONY:dump
dump:
	$(OBJDUMP) -D -m riscv $(ELF) > $(BUILD_DIR)/disassembly.asm

.PHONY:disk
disk:
	mkdir -p $(MOUNT_PATH)
	chmod +x tools/mkdisk.sh
	tools/mkdisk.sh $(DISK)

.PHONY:umount
umount:
	@ sudo umount $(MOUNT_PATH)

.PHONY:mount
mount:
	@ sudo mount $(DISK_DEV) $(MOUNT_PATH) && echo "挂载成功!" 

.PHONY:show
show:
	sudo mount $(DISK_DEV) $(MOUNT_PATH) && echo "挂载成功!"
	ls -la $(MOUNT_PATH)/
	sudo umount $(MOUNT_PATH)

.PHONY: move
move:
	sudo mount $(DISK_DEV) $(MOUNT_PATH) && echo "挂载成功!" 
	sudo cp $(TARGET) $(MOUNT_PATH)/
	sudo umount $(MOUNT_PATH)

.PHONY: u
u:
	$(MAKE) -C user_proc/proc1 all
	$(MAKE) -C user_proc/proc2 all

.PHONY: uc
uc:
	$(MAKE) -C user_proc/proc1 clean
	$(MAKE) -C user_proc/proc2 clean
	rm -rf user_proc/user/

#********************************************************************************
#qemu模拟器
QEMU = qemu-system-riscv64
QFLAGS = -nographic -smp 1 -machine virt -bios arch/$(ARCH)/boot/u-boot.bin -cpu rv64,sstc=on
QFLAGS += -drive file=$(DISK),if=none,format=raw,id=disk0
QFLAGS += -device virtio-blk-device,drive=disk0,bus=virtio-mmio-bus.0 
QFLAGS += -global virtio-mmio.force-legacy=false
#gdb
GDB = gdb-multiarch
GFLAGS = -tui -q -x gdbinit

.PHONY:run
run: build 
	@${QEMU} -M ? | grep virt >/dev/null || exit
	@echo "\033[32m先按 Ctrl+A 再按 X 退出 QEMU"
	@echo "------------------------------------\033[0m"
	${QEMU} ${QFLAGS}



# ext2load virtio 0:1 0x80200000 /kernel.bin
# ext2load virtio 0:1 0x80400000 /qemu_virt.dtb
# go 0x80200000