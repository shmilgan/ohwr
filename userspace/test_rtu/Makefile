CC=$(CROSS_COMPILE)gcc

WR_INSTALL_ROOT ?= $(WRS_OUTPUT_DIR)/images/wr

OBJS = rtu_sim.o rtu_hw.o rtu_test.o rtu_common.o \
	rtu_test_data.o rtu_test_main.o 

LDFLAGS =  -L$(WR_INSTALL_ROOT)/lib -lswitchhw

CFLAGS = -I. -O2 -I../include -DDEBUG
BINARY = rtu_sim

all:	$(OBJS)
	${CC} -o $(BINARY) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(BINARY) *.o *~

install:
	install -d $(WR_INSTALL_ROOT)/bin
	install $(BINARY) $(WR_INSTALL_ROOT)/bin
