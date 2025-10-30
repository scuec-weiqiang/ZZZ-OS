#--------------架构---------------#
ARCH ?= riscv64
CROSS_COMPILE ?= riscv64-unknown-elf-
BOARD ?= qemu_virt

#--------------输出目录---------------#
BUILD_DIR := build
TARGET := $(BUILD_DIR)/kernel.img

#--------------编译器---------------#
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJDUMP :=$(CROSS_COMPILE)objdump

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
all: $(ASM_LINK) $(KBUILD_FILE) $(TARGET) $(DTB)
	sudo mount -o loop ../disk.img ../mount && echo "挂载成功!" || dmesg | tail -n 20
	sudo cp $(TARGET) ../mount/
	sudo cp $(DTB) ../mount/
	sudo umount ../mount

os: $(KBUILD_FILE) $(TARGET)
	sudo mount -o loop ../disk.img ../mount && echo "挂载成功!" || dmesg | tail -n 20
	sudo cp $(TARGET) ../mount/
	sudo umount ../mount

$(TARGET): $(BUILD_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
	rm $(KBUILD_FILE)
	@echo "$@ is ready"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(dir $<) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -I$(dir $<) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
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
	sudo mount -o loop ../disk.img ../mount && echo "挂载成功!" || dmesg | tail -n 20
	sudo cp $(DTB) ../mount/
	sudo umount ../mount
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
	-rm -rf $(ASM_LINK)
	@echo "清理完成"



#********************************************************************************
.PHONY:dump
dump:
	$(OBJDUMP) -D -m riscv $(TARGET) > $(BUILD_DIR)/disassembly.asm

.PHONY:disk
disk:
	dd if=/dev/urandom of=../disk.img bs=1M count=16

.PHONY:disk_clean
disk_clean:
	rm ../disk.img

.PHONY:umount
umount:
	sudo umount ../mount

.PHONY:mount
mount:
	sudo mount -o loop ../disk.img ../mount && echo "挂载成功!" || dmesg | tail -n 20

.PHONY:format
format: 
	mkfs.ext2 -I 128 -F ../disk.img


.PHONY:show
show:
	sudo mount -o loop ../disk.img ../mount && echo "挂载成功!" || dmesg | tail -n 20
	ls -la ../mount/
	sudo umount ../mount

.PHONY: move
move:
	sudo mount -o loop ../disk.img ../mount && echo "挂载成功!" || dmesg | tail -n 20
	sudo cp $(TARGET) ../mount/
	sudo umount ../mount