
OBJS = hal_exports.o hal_main.o hal_ports.o hal_config.o hal_timing.o

BINARY = wrsw_hal

# # Standard stanza for cross-compilation (courtesy of the linux makefile)
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

# We must include stuff from wr_ipc, which is installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS = -O -g -Wall \
	-I../include \
	-I../libwr/include \
	-I../mini-rpc \
	-I$(LINUX)/arch/arm/mach-at91/include \
	-I$(WR_INCLUDE)

ifdef WRS_HAL_DEBUG
  CFLAGS += -DDEBUG
endif

LDFLAGS = -L../libwr -L../mini-rpc \
	 -lminipc -llua -lm -ldl -lwr 

all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -d        $(WR_INSTALL_ROOT)/bin $(WR_INSTALL_ROOT)/include
	install $(BINARY) $(WR_INSTALL_ROOT)/bin
	install ../include/hal/hal_exports.h wrsw_hal.h $(WR_INSTALL_ROOT)/include

clean:
	rm -f $(BINARY) *.o *~
