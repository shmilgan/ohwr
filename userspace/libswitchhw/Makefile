CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
CFLAGS = -I. -O2 -I../include -DDEBUG -I./minilzo -g -I./lua-5.2.0/src/

OBJS = trace.o init.o fpga_io.o util.o pps_gen.o i2c.o pio_pins.o i2c_bitbang.o i2c_fpga_reg.o pio.o libshw_i2c.o i2c_sfp.o
SCAN_OBJS = i2cscan.o

LIB = libswitchhw.a
LDFLAGS += -lm liblua.a

all: $(LIB) 

$(LIB): $(OBJS)
	$(AR) rc $@ $^

i2cscan: $(OBJS) $(LIB) $(SCAN_OBJS)
	$(CC) $(CFLAGS) $(SCAN_OBJS) -o $(I2CSCAN_TARGET) $(LDFLAGS) $(LIB) $(LDFLAGS)



I2CSCAN_TARGET=i2cscan

#all:	$(LIB) i2cscan

#$(LIB): $(OBJS)
#	$(AR) rc $@ $^

#i2cscan: $(LIB) i2cscan.o
#	$(CC) $(CFLAGS) liblua.a i2cscan.o liblua.a -o $(I2CSCAN_TARGET) $(LDFLAGS) -lm

dbload: dbload.o
	$(CC) -Wall -ggdb -Ilua-5.2.0/src dbload.c liblua.a -o dbload -lm

install: all
	install -d $(WR_INSTALL_ROOT)/lib
	install $(LIB) $(WR_INSTALL_ROOT)/lib

clean:
	rm -f $(LIB) $(OBJS) $(I2CSCAN_TARGET) dbload dbload.o

ctt:
	scp i2cscan root@192.168.10.9:/bart_i2cscan

cp: clean $(LIB) i2cscan
	cp i2cscan /tftpboot/rootfs/mvanga_i2cscan
	cp sfpdb.lua /tftpboot/rootfs/root/
	ssh root@192.168.1.11
