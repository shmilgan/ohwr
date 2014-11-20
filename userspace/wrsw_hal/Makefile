
CC = $(CROSS_COMPILE)gcc

OBJS = hal_exports.o hal_main.o hal_ports.o hal_config.o rt_client.o \
	hal_timing.o

BINARY = wrsw_hal

# We must include stuff from wr_ipc, which is installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS = -O -g -Wall -DRT_CLIENT\
	-I../include -I../libwr/include -I../mini-rpc -I$(WR_INCLUDE)

ifdef WRS_HAL_DEBUG
  CFLAGS += -DDEBUG
endif

LDFLAGS = -L../libwr -L../mini-rpc \
	 -lminipc -llua -lm -ldl -lwr 

all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -d        $(WR_INSTALL_ROOT)/bin $(WR_INSTALL_ROOT)/include
	install $(BINARY) $(WR_INSTALL_ROOT)/bin
	install ../include/hal/hal_exports.h wrsw_hal.h $(WR_INSTALL_ROOT)/include

clean:
	rm -f $(BINARY) *.o *~
