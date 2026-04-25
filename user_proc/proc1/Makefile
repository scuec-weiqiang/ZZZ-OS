ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

ROOT := ../..
APP := proc1

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

CFLAGS := -g -Wall -ffreestanding -fno-builtin -fno-pic -fno-pie -no-pie
CFLAGS += -I$(ROOT)/include -I$(ROOT)/user -I..
CFLAGS += -march=armv7-a -mfpu=vfpv3-d16 -fno-omit-frame-pointer -marm
ASFLAGS := $(CFLAGS)
LDFLAGS := -nostdlib -static -Wl,-T../link.ld

APP_OBJ := $(APP).o libgcc_compat.o
USER_OBJS := ../user/printf.arm.o ../user/usys.arm.o
TARGET := $(APP)
TARGET_BIN := $(APP).bin

all: $(TARGET)

$(TARGET): $(APP_OBJ) $(USER_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -lgcc -o $@
	sudo cp $(TARGET) ../../../linux/tftpboot/mnt/

$(TARGET_BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@

$(APP).o: $(APP).c
	$(CC) $(CFLAGS) -c $< -o $@

libgcc_compat.o: libgcc_compat.c
	$(CC) $(CFLAGS) -c $< -o $@

../user/printf.arm.o: $(ROOT)/user/printf.c $(ROOT)/user/printf.h $(ROOT)/user/usys.h
	@mkdir -p ../user
	$(CC) $(CFLAGS) -c $< -o $@

../user/usys.arm.o: $(ROOT)/user/usys.S $(ROOT)/user/usys.h $(ROOT)/user/syscall_num.h
	@mkdir -p ../user
	$(CC) $(ASFLAGS) -c $< -o $@

clean:
	rm -f $(APP_OBJ) $(TARGET) $(TARGET_BIN)
	rm -f ../user/printf.arm.o ../user/usys.arm.o

.PHONY: all clean
