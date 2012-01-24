
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
CFLAGS = -I. -O2 -I../include -DDEBUG -I./minilzo

OBJS = trace.o init.o fpga_io.o util.o pps_gen.o

LIB = libswitchhw.a

all:	$(LIB)

$(LIB): $(OBJS)
	$(AR) rc $@ $^

install: all
	install -d $(WR_INSTALL_ROOT)/lib
	install $(LIB) $(WR_INSTALL_ROOT)/lib

clean:
	rm -f $(LIB) $(OBJS)
