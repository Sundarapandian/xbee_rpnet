##########################################################
#                                                        #
#  TODO: Add file header here                            #
#                                                        #
#                                                        #
##########################################################
TARGET = xapp

ARCH ?= linux

DIRS := xbee \
		arch/$(ARCH)
CFLAGS ?= -Iarch/$(ARCH)/include -Ixbee/include -Wall
CC := $(CROSS_COMPILE)gcc
LD := $(CC)
AS := $(CC)
OBJCOPY  := $(CROSS_COMPILE)objcopy

LIBS := -lpthread
LDFLAGS := -static

include arch/$(ARCH)/config.mk

# Add Xbee sources
CSRC = $(foreach dir,$(DIRS),$(shell find $(dir) -name "*.c"))
ASRC = $(foreach dir,$(DIRS),$(shell find $(dir) -name "*.S"))
OBJS = $(subst .c,.o,$(CSRC))
OBJS += $(subst .S,.o,$(ASRC))

all: $(TARGET).elf $(TARGET).bin

$(TARGET).elf: $(OBJS) .deps
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) $(OCFLAGS) -O binary $^ $@

.PHONY: clean distclean

clean:
	@echo "Cleaning object files..."
	@-rm -f $(OBJS) $(TARGET)

distclean: clean
	@-rm -f .deps

.deps: $(CSRC)
	@echo "Creating dependencies..."
	@$(CC) $(CFLAGS) -MM $^ > $@

include .deps
