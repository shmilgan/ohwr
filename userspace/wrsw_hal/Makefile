
CC = $(CROSS_COMPILE)gcc

OBJS = hal_exports.o hal_main.o hal_ports.o hal_config.o rt_client.o hal_timing.o

BINARY = wrsw_hal

# We must include stuff from wr_ipc, which is installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS = -g -Wall -DDEBUG -DRT_CLIENT\
	-I../include -I../mini-rpc -I$(WR_INCLUDE)

LDFLAGS = -L$(WR_INSTALL_ROOT)/lib -L../3rdparty/lib -L$(WR_LIB) \
	 -L../libswitchhw -L../mini-rpc \
	 -lminipc -llua -lm -ldl -lswitchhw

all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -d         $(WR_INSTALL_ROOT)/bin
	install $(BINARY) $(WR_INSTALL_ROOT)/bin
	install hal_exports.h wrsw_hal.h $(WR_INSTALL_ROOT)/include

clean:
	rm -f $(BINARY) *.o *~
