
CFLAGS = -Wall -ggdb $(EXTRA_CFLAGS)

ifdef LINUX
    CFLAGS += -I$(LINUX)/include \
		-I$(LINUX)/arch/arm/mach-at91/include
endif

AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

LDFLAGS = -L. 

PROGS = mapper wmapper 

all: $(PROGS)

$(PROGS): 

