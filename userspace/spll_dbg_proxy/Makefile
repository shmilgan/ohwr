
CC = $(CROSS_COMPILE)gcc

OBJS = spll_dbg_proxy.o

BINARY = spll_dbg_proxy

# We must include stuff from wr_ipc, which is installed.
# If this is built under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS = -g -Wall -DDEBUG -I../include -I$(WR_INCLUDE)

LDFLAGS = -L$(WR_LIB) -L../libswitchhw -llua -lm -ldl -lswitchhw

all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BINARY) $(OBJS)

install:
	install -d $(WR_INSTALL_ROOT)/bin
	install $(BINARY) $(WR_INSTALL_ROOT)/bin


# deprecated: internal stuff.
scp: all
	scp $(BINARY) root@pts-test03:/tftpboot/rootfs/wr/bin

