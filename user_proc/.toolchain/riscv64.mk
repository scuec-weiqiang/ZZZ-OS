CROSS_COMPILE ?= riscv-none-elf-

USER_CPUFLAGS ?= -march=rv64gc -mabi=lp64d -mcmodel=medany -fno-omit-frame-pointer
USER_ASFLAGS ?= $(USER_CPUFLAGS)
