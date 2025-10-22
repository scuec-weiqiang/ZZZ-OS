include ../config.mk

SRCS := $(wildcard fs/*.c) $(wildcard fs/ext2/*.c)
OBJS := $(SRCS:%.c=$(OUT_DIR)/%.o)

$(OUT_DIR)/%.o: %.c
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OUT_DIR)/fs/built-in.o: $(OBJS)
	@echo "  AR      $@"
	$(LD) -r -o $@ $(OBJS)
