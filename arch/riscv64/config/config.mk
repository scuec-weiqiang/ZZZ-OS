CFLAGS += -march=rv64gc -mabi=lp64d -mcmodel=medany -fno-omit-frame-pointer
CFLAGS += -DSYS_BITS=64
LDFLAGS += -m elf64lriscv
MKIMAGE_ARCH := riscv
OBJDUMP_ARCH := riscv
