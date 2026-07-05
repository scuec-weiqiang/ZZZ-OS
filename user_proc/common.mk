ARCH ?= riscv64

COMMON_MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
ARCH_TOOLCHAIN_MK := $(COMMON_MK_DIR).toolchain/$(ARCH).mk

ifneq ($(wildcard $(ARCH_TOOLCHAIN_MK)),)
include $(ARCH_TOOLCHAIN_MK)
endif

ifndef CROSS_COMPILE
$(error Missing CROSS_COMPILE; set it in the parent Makefile or via $(ARCH_TOOLCHAIN_MK))
endif

CC := $(CROSS_COMPILE)gcc

BUILD_DIR ?= build/$(ARCH)
INSTALL_DIR ?= ../../mount/bin

CPPFLAGS += -I../../include
CFLAGS += $(USER_CPUFLAGS) -Wall -g -MMD -MP
LDFLAGS += $(USER_LDFLAGS)

ifeq ($(STATIC),0)
else
LDFLAGS += -static
endif

ifndef TARGET
$(error TARGET must be set before including ../common.mk)
endif

SRCS ?= $(TARGET).c
OBJS ?= $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

.PHONY: all install clean

ifeq ($(AUTO_INSTALL),0)
all: $(TARGET)
else
all: install
endif

install: $(TARGET)
	@test -d $(INSTALL_DIR) || \
		( echo "install target '$(INSTALL_DIR)' is not a directory; mount the image first" && exit 1 )
	sudo cp $(TARGET) $(INSTALL_DIR)/
	


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(OBJS:.o=.d)
