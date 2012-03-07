
CC = $(CROSS_COMPILE)gcc

OBJS = spll_dbg_proxy.o

BINARY = spll_dbg_proxy

# We must include stuff from wr_ipc, which is installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS = -g -Wall -DDEBUG \
	-I. -I../../../ptp-noposix-v3/libwripc -I../include -I../3rdparty/include -I$(WR_INCLUDE)

LDFLAGS = -L$(WR_INSTALL_ROOT)/lib -L../3rdparty/lib -L$(WR_LIB) \
	-L../../../ptp-noposix-v3 -L../libswitchhw \
	 -lwripc -llua -lm -ldl -lswitchhw

all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

scp: all
		scp $(BINARY) root@pts-test03:/tftpboot/192.168.1.2/root

clean:
		rm -f $(BINARY) $(OBJS)