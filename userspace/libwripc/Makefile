
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
CFLAGS = -I. -O2 -I../include

OBJS = helper_arm.o wr_ipc.o

LIB = libwripc.a

all:	$(LIB)

$(LIB): $(OBJS)
	$(AR) rc $@ $^

install: all
	install -d $(WR_INSTALL_ROOT)/lib
	install $(LIB) $(WR_INSTALL_ROOT)/lib

clean:
	rm -f $(LIB) $(OBJS)
