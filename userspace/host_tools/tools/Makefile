vpath %.c ../../tools

TOOLS += wrs_dump_shmem

# this is a host version of libwr, ignore CROSS_COMPILE
CROSS_COMPILE:=
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

CFLAGS = -Wall -I. -O2 -ggdb \
	-Wstrict-prototypes \
	-I../../libwr/include \
	-I../../include \
	-I../../ppsi/include \
	-I../../ppsi/arch-wrs/include \
	-I../../mini-rpc \

LDFLAGS = \
	-L../libwr \
	-lwr \
	-lrt \


all: $(TOOLS)

wrs_dump_shmem: wrs_dump_shmem.o
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	rm -f $(TOOLS) *.o
