
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
CFLAGS = -I. -O2 -I../include -I../wrsw_hal -I../mini-rpc -DDEBUG

OBJS = hal_client.o netif_test.o ptpd_netif.o

LIB = libptpnetif.a

all:	$(LIB)

$(LIB): $(OBJS)
	$(AR) rc $@ $^

install: all
	install -d $(WR_INSTALL_ROOT)/lib
	install $(LIB) $(WR_INSTALL_ROOT)/lib

clean:
	rm -f $(LIB) $(OBJS)
