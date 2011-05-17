CC=$(CROSS_COMPILE)gcc
OBJS = ptpd_netif_mch.o ptpd_netif_test.o
LDFLAGS = -lswitchhw -L../../../libswitchhw
CFLAGS = -I. -O2 -I../../../include -DDEBUG -I../../../../kernel/include
OUTPUT = dupa


all:	$(OBJS)
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run:  all
	scp $(OUTPUT) root@$(T):/tmp
	- ssh -t root@$(T) "/tmp/$(OUTPUT) wru1"

install: all
		mkdir -p ../../rootfs_override/root/tests
		cp $(OUTPUT) ../../rootfs_override/root/tests

clean:
	rm -f $(OUTPUT) $(OBJS)