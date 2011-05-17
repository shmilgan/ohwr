
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
CFLAGS = -I. -O2 -I../include -DDEBUG -I./minilzo

OBJS = pio.o pio_pins.o trace.o init.o fpga_io.o util.o ad9516.o \
	fpgaboot.o minilzo/minilzo.o clkb_io.o xpoint.o hpll.o \
	mblaster.o phy_calibration.o dmpll.o pps_gen.o watchdog.o

LIB = libswitchhw.a

all:	$(LIB)

$(LIB): $(OBJS)
	$(AR) rc $@ $^

install: all
	install -d $(WR_INSTALL_ROOT)/lib
	install $(LIB) $(WR_INSTALL_ROOT)/lib

clean:
	rm -f $(LIB) $(OBJS)
