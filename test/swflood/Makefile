AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

CFLAGS = -Wall -O2 -ggdb

ALL = swflood multidump

all: $(ALL)

swflood: main.o parse.o commands.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# build commands.o by various cmd-*.c using a linker script
CMD_C := $(wildcard cmd-*.c)
CMD_O := $(CMD_C:.c=.o)


commands.o: $(CMD_O)
	$(LD) -r -T commands.lds $^ -o $@

clean:
	rm -f $(ALL) *.o *~ core
