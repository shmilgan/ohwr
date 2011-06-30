# # Standard stanza for cross-compilation (courtesy of the linux makefile)
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump


MON_OBJS = wr_mon.o term.o 

CFLAGS += -O2 -DDEBUG -I$(LINUX)/include

ALL = wr_mon

all: $(ALL)

wr_mon: $(MON_OBJS)
	${CC} $^  $(LDFLAGS) -o $@ 

clean:
	rm -f $(ALL) *.o *~