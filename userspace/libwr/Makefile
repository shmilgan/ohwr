CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
CFLAGS = -Wall -I. -O2  -DDEBUG -ggdb -I./include -I../include \
	-I../wrsw_hal -I../mini-rpc

OBJS = trace.o init.o fpga_io.o util.o pps_gen.o i2c.o shw_io.o i2c_bitbang.o \
       i2c_fpga_reg.o pio.o libshw_i2c.o i2c_sfp.o fan.o i2c_io.o hwiu.o \
       ptpd_netif.o hal_client.o shmem.o

LIB = libwr.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rc $@ $^

install: all
	install -d $(WR_INSTALL_ROOT)/lib
	install $(LIB) $(WR_INSTALL_ROOT)/lib

clean:
	rm -f $(LIB) $(OBJS)
