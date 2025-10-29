CC = gcc
CFLAGS =  -Wall -g

TARGET := kbuild

kbuild:  $(TARGET)

$(TARGET): kbuild.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
