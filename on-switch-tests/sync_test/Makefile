include ../../../Makedefs

CC=$(CROSS_COMPILE_ARM)gcc
OBJS = hal_client.o sync_test.o ptpd_netif_mch.o
LDFLAGS = -lswitchhw -L../../libswitchhw -L../../libwripc -lwripc
CFLAGS = -I. -O2 -I../../include -DDEBUG -I../../../kernel/include -I../../wrsw_hal -I../../libwripc -g
OUTPUT = sync_test


all:	$(OBJS)
	make -C ../../libswitchhw
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run:  all
	- scp $(OUTPUT)  root@192.168.1.6:/tmp
	- scp $(OUTPUT)  root@192.168.1.13:/tmp


install: all
		mkdir -p ../../rootfs_override/root/tests
		cp $(OUTPUT) ../../rootfs_override/root/tests

clean:
	rm -f $(OUTPUT) $(OBJS)