vpath %.c ../../libwr

OBJS = init.o fpga_io.o util.o pps_gen.o i2c.o shw_io.o i2c_bitbang.o \
       i2c_fpga_reg.o pio.o libshw_i2c.o i2c_sfp.o fan.o i2c_io.o hwiu.o \
       ptpd_netif.o hal_client.o \
       shmem.o rt_client.o \
       dot-config.o wrs-msg.o \
       mac.o

LIB = libwr.a
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
	-DLIBWR_INTERNAL \
	-I../../libwr/include \
	-I../../include \
	-I../../mini-rpc \
	-I.

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rc $@ $^


clean:
	rm -f $(LIB) $(OBJS)
