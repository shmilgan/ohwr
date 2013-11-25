
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

CFLAGS = -Wall -ggdb -O2 -I../wrsw_hal -I../mini-rpc

all: libptpnetif.a

libptpnetif.a: ptpd_netif.o hal_client.o
	$(AR) r $@ $^


clean:
	rm -f *.a *.o *~

install:
	@echo "We have no install rule by now"
