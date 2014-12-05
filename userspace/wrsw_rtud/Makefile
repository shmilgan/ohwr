
PROGRAM		= wrsw_rtud
SRCFILES	= mac.c rtu_drv.c rtu_ext_drv.c rtu_hash.c rtu_fd.c rtud.c \
			rtud_exports.c utils.c
OBJFILES	= $(patsubst %.c,%.o,$(SRCFILES))

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

CFLAGS = -O2 -Wall -ggdb  \
	-I../mini-rpc \
	-I../include \
	-I../libwr/include \
	-I$(LINUX)/arch/arm/mach-at91/include

ifdef WRS_RTUD_DEBUG
  CFLAGS += -DDEBUG
endif

LDFLAGS 	:= -L../libwr -L../mini-rpc\
		-lwr -lpthread -lminipc

all: $(PROGRAM)

$(PROGRAM): $(OBJFILES)
	$(CC) -o $@ $(OBJFILES) $(LDFLAGS) 

clean:
	rm -f $(PROGRAM) *.o *~

install: all
	install -d $(WR_INSTALL_ROOT)/bin
	install $(PROGRAM) $(WR_INSTALL_ROOT)/bin
