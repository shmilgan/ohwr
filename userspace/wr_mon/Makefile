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


MON_OBJS = wr_mon.o term.o 

CFLAGS += -O2 -DDEBUG -I$(LINUX)/include -I../include \
	-I../libptpnetif -I../wrsw_hal -I../libwripc

LDFLAGS =  -L$(WR_INSTALL_ROOT)/lib -lptpnetif -lwripc

BINARY = wr_mon

all: $(BINARY)

wr_mon: $(MON_OBJS)
	${CC} $^  $(LDFLAGS) -o $@ 

clean:
	rm -f $(BINARY) *.o *~

install:
	install -d $(WR_INSTALL_ROOT)/bin
	install $(BINARY) $(WR_INSTALL_ROOT)/bin
