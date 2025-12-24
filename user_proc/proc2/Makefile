#--------------架构---------------#
ARCH ?= riscv64
CROSS_COMPILE ?= riscv64-unknown-elf-

#--------------输出目录---------------#
BUILD_DIR := build
PROJ_NAME = $(notdir $(shell pwd))
TARGET := $(BUILD_DIR)/$(PROJ_NAME).elf

#--------------编译器---------------#
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJDUMP :=$(CROSS_COMPILE)objdump

CFLAGS = -g -Wall -fno-builtin -mcmodel=medany 
CFLAGS += -I../../user/ -I../../include/
CFLAGS += -MMD -MP
ASFLAGS := $(CFLAGS)
LDFLAGS := -T../link.ld 
LDFLAGS += -nostdlib  # 不链接标准库
LDFLAGS += -nostartfiles  # 不使用标准启动文件（如 crt0.o）
LDFLAGS += -ffreestanding  # 告知编译器这是独立环境（无 OS）


#--------------分层构建---------------#
KBUILD_PATH = ../../tools/kbuild
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

MOUNT_PATH = ../../../mount/
DISK_PATH = ../../../disk.img

#--------------通用编译---------------#
all: $(TARGET) 
	sudo mount -o loop $(DISK_PATH) $(MOUNT_PATH) && echo "挂载成功!" || dmesg | tail -n 20
	sudo cp $(TARGET) $(MOUNT_PATH)
	sudo umount $(MOUNT_PATH)

$(TARGET): $(BUILD_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
	@echo "$@ is ready"

$(BUILD_DIR)/%.o: %.c $(KBUILD_FILE)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(dir $<) -c $< -o $@

$(BUILD_DIR)/%.o: %.S  $(KBUILD_FILE)
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -I$(dir $<) -c $< -o $@

$(BUILD_DIR)/%.o: %.s  $(KBUILD_FILE)
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


.PHONY: distclean
distclean:
	@echo "正在清理所有输出文件："
	-rm -rf $(BUILD_DIR)
	-$(MAKE) -C $(KBUILD_PATH) clean
	@echo "清理完成"



#********************************************************************************
.PHONY:dump
dump:
	$(OBJDUMP) -D -m riscv $(TARGET) > $(BUILD_DIR)/disassembly.asm