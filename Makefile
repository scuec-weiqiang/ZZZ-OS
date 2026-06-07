#--------------架构---------------#
ARCH ?= riscv64
CROSS_COMPILE ?= riscv-none-elf-
BOARD ?= qemu_virt

ARCH_CONFIG_MK := arch/$(ARCH)/config/config.mk
ifeq ($(wildcard $(ARCH_CONFIG_MK)),)
$(error Missing architecture config '$(ARCH_CONFIG_MK)')
endif

CONFIG_FILE ?= .config.$(ARCH)
DEPLOY_DIR ?= $(BUILD_DIR)/deploy/$(ARCH)/$(BOARD)
DEPLOY_BOOT_CMD := $(DEPLOY_DIR)/boot.cmd
DEPLOY_BOOT_SCR := $(DEPLOY_DIR)/boot.scr

#--------------输出目录---------------#
BUILD_DIR := build
TARGET := $(BUILD_DIR)/uImage
BIN := $(BUILD_DIR)/kernel.bin
ELF := $(BUILD_DIR)/kernel.elf
ELF_TMP := $(BUILD_DIR)/kernel.tmp.elf
KALLSYMS_SRC := $(BUILD_DIR)/kallsyms_data.c
KALLSYMS_OBJ := $(BUILD_DIR)/kallsyms_data.o
KALLSYMS_STUB_SRC := $(BUILD_DIR)/kallsyms_stub.c
KALLSYMS_STUB_OBJ := $(BUILD_DIR)/kallsyms_stub.o

#--------------编译器---------------#
CFLAGS = -g -Wall -fno-builtin -std=c11 -ffreestanding -fno-pic -fno-pie -no-pie 
LDFLAGS = -Tarch/$(ARCH)/config/link.ld 

include $(ARCH_CONFIG_MK)
ifndef CROSS_COMPILE
$(error Missing CROSS_COMPILE; pass CROSS_COMPILE=<toolchain-prefix>)
endif
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
NM := $(CROSS_COMPILE)nm
OBJDUMP :=$(CROSS_COMPILE)objdump
OBJCOPY := $(CROSS_COMPILE)objcopy
CFLAGS += -Iinclude 
CFLAGS += -MMD -MP
ASFLAGS := $(CFLAGS)

export ARCH
export CROSS_COMPILE

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
KBUILD_FILE = objs.$(ARCH).mk
KBUILD_SOURCES := $(shell find . -name objs.build)
# 若不存在 objs.mk，则生成

$(KBUILD_FILE):$(KBUILD) $(KBUILD_SOURCES) $(CONFIG_FILE)
	@$(KBUILD) $(SRC_ROOT) $(CONFIG_FILE) > $@
$(KBUILD):
	$(MAKE) -C $(KBUILD_PATH)
$(CONFIG_FILE):
	@cat arch/$(ARCH)/config/defconfig > $@
include $(KBUILD_FILE)
# 生成 .o 文件路径
BUILD_OBJS := $(patsubst %.o, $(BUILD_DIR)/%.o, $(OBJ_Y))


#--------------设备树---------------#
DTC_PATH = tools/dtc
DTC = $(DTC_PATH)/dtc
DTS :=  arch/$(ARCH)/boot/dts/$(BOARD).dts
DTB := $(BUILD_DIR)/$(DTS:.dts=.dtb)
$(DTB):$(DTC)

$(DTC):
	$(MAKE) -C ./tools/dtc

#--------------通用编译---------------#
all: os

artifacts: $(TARGET) $(DTB)

dist: artifacts
	@mkdir -p $(DEPLOY_DIR)
	cp $(TARGET) $(DEPLOY_DIR)/
	cp $(DTB) $(DEPLOY_DIR)/
	tools/deploy/generate_bootscript.sh \
		--arch $(ARCH) \
		--board $(BOARD) \
		--dtb $(notdir $(DTB)) \
		--output-dir $(DEPLOY_DIR)

os: dist
	@echo "boot artifacts are ready in $(DEPLOY_DIR)"

$(TARGET): $(BIN)
	./tools/mkimage -A $(patsubst riscv64,riscv,$(ARCH)) -O linux -T kernel -C none -a 0x80200000 -e 0x80200000 -n "ZZZ-OS" -d $(BIN) $(TARGET)


$(BIN): $(ELF) 
	$(OBJCOPY) -O binary $< -S $@
	@echo "$@ is ready"
	
$(KALLSYMS_STUB_SRC):
	@mkdir -p $(dir $@)
	@printf '%s\n' '#include <os/kallsyms.h>' > $@
	@printf '%s\n' 'const struct kernel_symbol __kallsyms[] = { { 0, 0 } };' >> $@
	@printf '%s\n' 'const unsigned int __kallsyms_count = 0;' >> $@

$(KALLSYMS_STUB_OBJ): $(KALLSYMS_STUB_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF_TMP): $(BUILD_OBJS) $(KALLSYMS_STUB_OBJ)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $^ -o $@

$(KALLSYMS_SRC): $(ELF_TMP) tools/gen_kallsyms.sh
	@mkdir -p $(dir $@)
	chmod +x tools/gen_kallsyms.sh
	NM="$(NM)" tools/gen_kallsyms.sh $(ELF_TMP) $@

$(KALLSYMS_OBJ): $(KALLSYMS_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(BUILD_OBJS) $(KALLSYMS_OBJ)
	$(LD) $(LDFLAGS) $^ -o $@
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
	rm -rf $(BUILD_DIR) $(KBUILD_FILE) $(CONFIG_FILE)
	$(MAKE) -C ./user_runtime/ clean
	@echo "clean complete"

.PHONY: all clean artifacts dist os

-include $(BUILD_OBJS:.o=.d)



#--------------kbuild工具编译---------------#
kbuild: $(KBUILD)

clean_kbuild:
	$(MAKE) -C $(KBUILD_PATH) clean


#--------------设备树编译---------------#
dtbs: $(DTB)
clean_dtbs:
	rm -f *.dtb
	

$(BUILD_DIR)/%.dtb: %.dts
	mkdir -p $(BUILD_DIR)/arch/$(ARCH)/boot/dts
	$(CC) -E -nostdinc -undef -D__DTS__ -x assembler-with-cpp -Iinclude -o - $< \
	| $(DTC) -I dts -O dtb -o $@ -i . -

.PHONY: dtc
dtc:
	$(MAKE) -C tools/dtc 

.PHONY: distclean
distclean:
	@echo "正在清理所有输出文件："
	make clean
	make uc
	-rm -rf $(BUILD_DIR) objs.*.mk
	-$(MAKE) -C $(DTC_PATH) clean
	-$(MAKE) -C $(KBUILD_PATH) clean
	-rm -f .config .config.*
	-rm -rf $(ASM_LINK)
	@echo "清理完成"



#********************************************************************************
.PHONY: install
install:
	tools/deploy/install_disk_image.sh --image build/images/qemu_virt.img --arch riscv64 --cross-compile riscv-none-elf- --board qemu_virt

.PHONY: run
run:
	tools/deploy/run_qemu_riscv64.sh

.PHONY:dump
dump:
	$(OBJDUMP) -D -m $(patsubst riscv64,riscv,$(ARCH)) $(ELF) > $(BUILD_DIR)/disassembly.asm

.PHONY: u
u:
	@set -e; \
	for d in user_proc/*; do \
		if [ -d "$$d" ] && [ -f "$$d/Makefile" ]; then \
			$(MAKE) -C "$$d" all; \
		fi; \
	done

.PHONY: uc
uc:
	@set -e; \
	for d in user_proc/*; do \
		if [ -d "$$d" ] && [ -f "$$d/Makefile" ]; then \
			$(MAKE) -C "$$d" clean; \
		fi; \
	done
	rm -rf user_proc/user/
