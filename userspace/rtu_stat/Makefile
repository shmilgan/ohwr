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

OBJS = rtu_stat.o

CFLAGS =  -O2 -I../include -DDEBUG -I$(LINUX)/include -I../libptpnetif \
	-I../wrsw_hal -I../libwripc -I../wrsw_rtud -g 

LDFLAGS =  -L$(WR_INSTALL_ROOT)/lib -lptpnetif -lwripc -lswitchhw 

BINARY = rtu_stat


all: $(BINARY)

$(BINARY): $(OBJS)
	${CC} $^  $(LDFLAGS) -o $@ 

clean:
	rm -f $(BINARY) *.o *~

install:
	install -d $(WR_INSTALL_ROOT)/bin
	install $(BINARY) $(WR_INSTALL_ROOT)/bin

# local deprecated rules follow, don't use
run:  all
	- scp $(BINARY)  root@192.168.1.16:/tmp
	- scp $(BINARY)  root@192.168.1.13:/tmp

deploy: all
		scp $(BINARY) $(WR_INSTALL_ROOT)/bin