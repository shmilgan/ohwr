CC = ~/wrs-build/build/buildroot-2011.11/output/host/usr/bin/arm-linux-gcc
AR = ~/wrs-build/build/buildroot-2011.11/output/host/usr/bin/arm-linux-ar
CFLAGS = -I. -O2 -I../include -DDEBUG -I./minilzo -g

OBJS = trace.o init.o fpga_io.o util.o pps_gen.o i2c.o pio_pins.o i2c_cpu_bb.o i2c_fpga_reg.o pio.o libshw_i2c.o

LIB = libswitchhw.a

I2CSCAN_TARGET=i2cscan

all:	$(LIB) i2cscan

$(LIB): $(OBJS)
	$(AR) rc $@ $^

i2cscan: $(LIB) i2cscan.o
	$(CC) $(CFLAGS) i2cscan.c -o $(I2CSCAN_TARGET) $(LIB)

install: all
	install -d $(WR_INSTALL_ROOT)/lib
	install $(LIB) $(WR_INSTALL_ROOT)/lib

clean:
	rm -f $(LIB) $(OBJS) $(I2CSCAN_TARGET)

ctt:
	scp i2cscan root@192.168.10.9:/bart_i2cscan