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
		# user \

PROJ_NAME = $(notdir $(shell pwd))
DIR_SOURCES = $(DIR)     
DIR_INCLUDE = $(addprefix -I, $(DIR))     
DIR_OUT     = out
TARGET      = $(DIR_OUT)/$(PROJ_NAME).elf
LINK_SCRIPT = arch/$(ARCH)/$(BOARD)/os.ld

# 编译工具链配置
CC = riscv64-unknown-elf-gcc
CFLAGS = -nostdlib -fno-builtin -g -Wall \
        $(DIR_INCLUDE) -march=rv64gc -mabi=lp64d -mcmodel=medany \
		-MMD -MP -MT $@ -MF $(DIR_OUT)/$*.d  # 添加依赖追踪选项
# CFLAGS += -O1
LD = riscv64-unknown-elf-ld
LFLAGS = -T$(LINK_SCRIPT) -Map=$(DIR_OUT)/$(PROJ_NAME).map
# 默认情况下，编译工程里的全部文件；
# 在某些情况下（比如有的模块只写了一半编译无法通过但是又需要半路去测试其他模块了，或者我完全不需要编译一些模块）
# 可以只编译在part.cfg文件中指明的文件，便于测试，在shell中输入make <你的其他指令> PART=1,就可以开启部分编译
PART ?= 0
CONFIG ?= part.cfg



ifeq ($(PART),1)#如果开启部分编译
    ifneq ($(wildcard $(CONFIG)),)# 且文件不为空	
        include $(CONFIG)
        $(warning 开启部分编译模式)
        SRC_C   = $(wildcard $(FILES:.S=.c))
        SRC_ASM = $(wildcard $(FILES:.c=.S))
    else
        $(warning 未找到配置文件: $(CONFIG))
        $(warning 退回到全部编译模式) 
        SRC_C   = $(foreach dir, $(DIR_SOURCES), $(wildcard $(dir)/*.c))
        SRC_ASM = $(foreach dir, $(DIR_SOURCES), $(wildcard $(dir)/*.S))
    endif
else
 	$(warning 全部编译模式) 
    SRC_C   = $(foreach dir, $(DIR_SOURCES), $(wildcard $(dir)/*.c))
    SRC_ASM = $(foreach dir, $(DIR_SOURCES), $(wildcard $(dir)/*.S))
endif

# 生成目标文件列表（保持目录结构）
OBJ = $(patsubst %.c, $(DIR_OUT)/%.o, $(SRC_C))
OBJ += $(patsubst %.S, $(DIR_OUT)/%.o, $(SRC_ASM))
DEP = $(OBJ:.o=.d) 
# 设置源文件搜索路径
vpath %.c $(sort $(dir $(SRC_C)))
vpath %.S $(sort $(dir $(SRC_ASM)))

# 包含依赖文件
-include $(DEP)

#*****************************************************************************************************

# 构建目标
os: $(TARGET)

$(TARGET): $(OBJ)
	@echo "\033[32m正在链接......\033[0m"
	$(LD) $(LFLAGS)  $^ -o $@
	@echo "\033[32m生成目标文件: $@\033[0m"


# $(DIR_OUT)/trap.o: trap.c
# 	@mkdir -p $(dir $@)
# 	@echo "\033[32m编译C文件: $<\033[0m"
# 	$(CC) $(CFLAGS)  -ffixed-x8 -ffixed-x9 -ffixed-x18 -ffixed-x19 -ffixed-x20 -ffixed-x21 -ffixed-x22 -ffixed-x23 -ffixed-x24 -ffixed-x25 -ffixed-x26 -ffixed-x27 -c $< -o $@

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
QFLAGS = -nographic -smp 1 -machine virt -kernel $(TARGET) -cpu rv64
QFLAGS += -drive file=disk.img,if=none,format=raw,id=disk0,cache=writeback
QFLAGS += -device virtio-blk-device,drive=disk0,bus=virtio-mmio-bus.0
QFLAGS += -d guest_errors,unimp,trace:time_memory*
QFLAGS += -global virtio-mmio.force-legacy=false
#gdb
GDB = gdb-multiarch
GFLAGS = -tui -q -x gdbinit

.PHONY:run
run: os 
	@${QEMU} -M ? | grep virt >/dev/null || exit
	@echo "\033[32m先按 Ctrl+A 再按 X 退出 QEMU"
	@echo "------------------------------------\033[0m"
	${QEMU} ${QFLAGS} -kernel $(TARGET) 

.PHONY:debug
debug:os
	@${QEMU} ${QFLAGS} -kernel $(TARGET)  -s -S &
	@${GDB}  ${GFLAGS} ${TARGET}  

.PHONY:qemu
qemu:os
	@${QEMU} ${QFLAGS}   -s -S 
	
.PHONY:gdb
gdb:os
	@$(GDB) $(TARGET)  -tui -q -x gdbinit

.PHONY:disk
disk:
	dd if=/dev/urandom of=disk.img bs=1M count=16

.PHONY:disk_clean
disk_clean:
	rm disk.img

.PHONY:dump
dump:
	riscv64-unknown-elf-objdump -D -m riscv $(TARGET) > $(DIR_OUT)/disassembly.asm

.PHONY:user
user:
	riscv64-unknown-elf-gcc user/user.c -T./user/user.ld -o user.elf

.PHONY:user_clean
user_clean:
	rm -f user/user.o user/user_program.elf user/user_program.elf lib/user_program.h

.PHONY:umount
umount:
	sudo umount ./mount

.PHONY:mount
mount:
	sudo mount -o loop ./disk.img ./mount && echo "挂载成功!" || dmesg | tail -n 20

.PHONY:format
format: 
	mkfs.ext2 -I 128 -F ./disk.img

.PHONY:show
show:
	sudo mount -o loop ./disk.img ./mount && echo "挂载成功!" || dmesg | tail -n 20
	ls -la ./mount/
	sudo umount ./mount
