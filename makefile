#*******************************************************************************
# 注意，事实上许多语句是不能有tab缩进的，但是由于我这里很多地方用的是4个空格，所以没问题
#*******************************************************************************

# 添加你的源文件目录
ARCH ?= riscv64
BOARD?= qemu_virt

DIR = 	kernel \
		kernel/fs \
		kernel/fs/ext2/ \
		kernel/fs/vfs/ \
        arch/$(ARCH)\
        arch/$(ARCH)/$(BOARD)\
		drivers \
		lib \

PROJ_NAME = $(notdir $(shell pwd))
DIR_SOURCES = $(DIR)     
DIR_INCLUDE = $(addprefix -I, $(DIR))     
DIR_OUT     = out
TARGET      = $(DIR_OUT)/$(PROJ_NAME).img
LINK_SCRIPT = arch/$(ARCH)/$(BOARD)/os.ld

# 编译工具链配置
CC = riscv64-unknown-elf-gcc
CFLAGS = -nostdlib -fno-builtin -g -Wall \
        $(DIR_INCLUDE) -march=rv64gc -mabi=lp64d -mcmodel=medany \
		-MMD -MP -MT $@ -MF $(DIR_OUT)/$*.d  # 添加依赖追踪选项

# CFLAGS += -O1
LD = riscv64-unknown-elf-ld
LFLAGS = -T$(LINK_SCRIPT) -Map=$(DIR_OUT)/$(PROJ_NAME).map

SRC_C   = $(foreach dir, $(DIR_SOURCES), $(wildcard $(dir)/*.c))
SRC_ASM = $(foreach dir, $(DIR_SOURCES), $(wildcard $(dir)/*.S))

# 生成目标文件列表（保持目录结构）
OBJ = $(patsubst %.c, $(DIR_OUT)/%.o, $(SRC_C))
OBJ += $(patsubst %.S, $(DIR_OUT)/%.o, $(SRC_ASM))
DEP = $(OBJ:.o=.d) 

# 设置源文件搜索路径
vpath %.c $(sort $(dir $(SRC_C)))
vpath %.S $(sort $(dir $(SRC_ASM)))

# 包含依赖文件
-include $(DEP)

# 构建目标
build: $(TARGET)

# 链接
$(TARGET): $(OBJ)
	@echo "\033[32m正在链接......\033[0m"
	$(LD) $(LFLAGS)  $^ -o $@
	@echo "\033[32m生成目标文件:\033[0m"
	@echo "\033[32m$@\033[0m"

# 通用编译规则
$(DIR_OUT)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "\033[32m编译C文件: $<\033[0m"
	$(CC) $(CFLAGS) -c $< -o $@

$(DIR_OUT)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "\033[32m编译汇编文件: $<\033[0m"
	$(CC) $(CFLAGS) -c $< -o $@

# 清理规则
.PHONY: clean
clean:
	@echo "正在清理所有输出文件："
	rm -rf $(DIR_OUT)
	@echo "清理完成"



#********************************************************************************
#qemu模拟器
# QEMU = qemu-system-riscv64
QEMU = /usr/local/qemu-riscv/bin/qemu-system-riscv64
QFLAGS = -nographic -smp 1 -machine virt -bios none -kernel $(TARGET) -cpu rv64
QFLAGS += -drive file=disk.img,if=none,format=raw,id=disk0,cache=writeback
QFLAGS += -device virtio-blk-device,drive=disk0,bus=virtio-mmio-bus.0
QFLAGS += -d guest_errors,unimp,trace:time_memory*
QFLAGS += -global virtio-mmio.force-legacy=false
#gdb
GDB = gdb-multiarch
GFLAGS = -tui -q -x gdbinit

.PHONY:debug
debug:os
	@${QEMU} ${QFLAGS}  -s -S &
	@${GDB}  ${GFLAGS} ${TARGET}  

.PHONY:qemu
qemu:os
	@${QEMU} ${QFLAGS}  -s -S 
	
.PHONY:gdb
gdb:os
	@$(GDB) $(TARGET)  -tui -q -x gdbinit

.PHONY:dump
dump:
	riscv64-unknown-elf-objdump -D -m riscv $(TARGET) > $(DIR_OUT)/disassembly.asm

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