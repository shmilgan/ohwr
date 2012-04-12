PROGRAM	= wrsw_mvrpd
OBJS    = mvrp_proxy.o mvrp_srv.o mvrp.o

CC      = $(CROSS_COMPILE)gcc

# We must include stuff from wr_ipc, which is installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS  := -O2 -DDEBUG -Wall -ggdb -DTRACE_ALL \
           -I. -I../include -I../wrsw_rtud -I../wrsw_mrp -I$(WR_INCLUDE) \
           -I$(LINUX)/include

LDFLAGS := -L. -L$(WR_LIB) \
           -lswitchhw -lwripc -lminipc -lwrrtu -lwrmrp

RM := rm -f

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	$(RM) $(OBJS)

install: all
	cp $(PROGRAM) $(WR_INSTALL_ROOT)/bin
