include ../../../Makedefs

CC=$(CROSS_COMPILE_ARM)gcc
OBJS = hal_client.o 
LDFLAGS = -lswitchhw -L../../libswitchhw -L../../libwripc -lwripc
CFLAGS = -I. -O2 -I../../include -I../../wrsw_hal -I../../libwripc -DDEBUG 
OUTPUT = hal_client


all:	$(OBJS)
	make -C ../../libswitchhw
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run:  all
	scp $(OUTPUT) ../../drivers/bin/*.ko root@$(T):/tmp
	- ssh -t root@$(T) "/tmp/$(OUTPUT)"

install: all
		mkdir -p ../../rootfs_override/root/tests
		cp $(OUTPUT) ../../rootfs_override/root/tests

clean:
	rm -f $(OUTPUT) $(OBJS)