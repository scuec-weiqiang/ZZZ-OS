CC = arm-none-eabi-gcc
CFLAGS = -mcpu=arm7tdmi -mfloat-abi=soft -marm -Wall -g
RUNTIME = ../../user_runtime


TARGET = hello
OBJS = $(TARGET).o

all: $(TARGET)
	sudo cp $(TARGET) ../../../linux/tftpboot/mnt/bin

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) \
		-nostartfiles \
		-T $(RUNTIME)/user.ld \
		$(RUNTIME)/crt0.o $(RUNTIME)/syscall.o $(RUNTIME)/syscalls.o \
		$(OBJS) -lc -lgcc -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
