include ../../Makedefs

CC=$(CROSS_COMPILE_ARM)gcc
AR=$(CROSS_COMPILE_ARM)ar
OBJS = pio.o pio_pins.o trace.o init.o fpga_io.o util.o ad9516.o fpgaboot.o minilzo/minilzo.o clkb_io.o xpoint.o hpll.o mblaster.o phy_calibration.o dmpll.o pps_gen.o watchdog.o

CFLAGS = -I. -O2 -I../include -I../include -DDEBUG -I./minilzo
OUTPUT = libswitchhw.a

all:	$(OBJS)
	${AR} rc $(OUTPUT) $(OBJS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS) -o $@

install:all
	scp $(OUTPUT) root@192.168.1.2:/root

clean:
	rm -f $(OUTPUT) $(OBJS)
	
deploy:
	