
OBJS = hal_exports.o hal_main.o hal_ports.o hal_timing.o

BINARY = wrsw_hal

WR_INSTALL_ROOT ?= /usr/lib/white-rabbit

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

CFLAGS = -O -g -Wall \
	-I../include \
	-I../libwr/include \
	-I../mini-rpc \
	-I$(LINUX)/arch/arm/mach-at91/include

LDFLAGS = -L../libwr -L../mini-rpc \
	 -lminipc -lm -ldl -lwr 

all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -d        $(WR_INSTALL_ROOT)/bin
	install $(BINARY) $(WR_INSTALL_ROOT)/bin

clean:
	rm -f $(BINARY) *.o *~
