include ../../Makedefs

PROGRAM		:= wrsw_rtud
SRCFILES    := mac.c rtu_drv.c rtu_hash.c rtu_fd.c rtud.c rtud_exports.c utils.c
OBJFILES    := $(patsubst %.c,%.o,$(SRCFILES))

CC		    := $(CROSS_COMPILE_ARM)gcc
INCLUDES    := -I. -I../include -I../rubi-repos/ptp-noposix/libptpnetif -I../wrsw_hal -I../rubi-repos/ptp-noposix/libwripc \
-I../../kernel/include -I$(CROSS_COMPILE_ARM_PATH)/../include
CFLAGS 		:= -O2 -DDEBUG -Wall -ggdb -DTRACE_ALL
LDFLAGS 	:= -L../libswitchhw -L../rubi-repos/ptp-noposix -lswitchhw \
-lptpnetif  -lwripc -lpthread

RM 		    := rm -f


.PHONY: all

all: $(PROGRAM)

$(PROGRAM): $(OBJFILES)
	$(CC) -o $@ $(OBJFILES) $(LDFLAGS) 

run:	all
	scp $(PROGRAM) root@$(T):/tmp
	
clean:
	$(RM) $(PROGRAM) $(OBJFILES)

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

deploy: all
	cp $(PROGRAM) $(WR_INSTALL_ROOT)/bin
