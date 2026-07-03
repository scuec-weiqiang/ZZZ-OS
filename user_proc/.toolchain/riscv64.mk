CROSS_COMPILE = riscv64-unknown-linux-gnu-

USER_CPUFLAGS ?= -march=rv64gc -mabi=lp64d -mcmodel=medany -fno-omit-frame-pointer
USER_ASFLAGS ?= $(USER_CPUFLAGS)
