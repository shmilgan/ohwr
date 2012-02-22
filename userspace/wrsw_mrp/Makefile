LIB  = libwrmrp.so
OBJS = wrsw_hal_ipc.o if_index.o mrp_socket.o mrp_pdu.o mrp.o

CC   = $(CROSS_COMPILE)gcc

# We must include stuff from wr_ipc, which is installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS := -fPIC -shared -O2 -DDEBUG -Wall -ggdb -DTRACE_ALL \
         -I. -I../include -I../wrsw_rtud -I$(WR_INCLUDE) -I$(LINUX)/include \
         -I../wrsw_hal

LDFLAGS := -fPIC -shared -L. -L$(WR_LIB) -lrt -lwripc -lminipc -lwrrtu

RM := rm -f

all: $(LIB)

$(LIB): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%: %.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(wildcard *.o): $(wildcard *.h)

clean:
	$(RM) $(OBJS)

install: all
	install -d $(WR_INSTALL_ROOT)/lib
	install $(LIB) $(WR_INSTALL_ROOT)/lib
