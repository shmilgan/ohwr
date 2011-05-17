include ../../../Makedefs

CC=$(CROSS_COMPILE_ARM)gcc
OBJS = ep_stats.o term.o
LDFLAGS = -lswitchhw -L../../libswitchhw -L../../libwripc -L../../libptpnetif -lwripc -lptpnetif
CFLAGS = -I. -O2 -I../../include -DDEBUG -I../../../kernel/include -I../../libptpnetif -I../../wrsw_hal -I../../libwripc -g
OUTPUT = ep_stats


all:	$(OBJS)
	make -C ../../libswitchhw
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run:  all
	- scp $(OUTPUT)  root@192.168.1.6:/tmp


install: all
		mkdir -p ../../rootfs_override/root/tests
		cp $(OUTPUT) ../../rootfs_override/root/tests

clean:
	rm -f $(OUTPUT) $(OBJS)