include ../../../Makedefs

CC=$(CROSS_COMPILE_ARM)gcc
OBJS = spi_test.o
LDFLAGS = -lswitchhw -L../../libswitchhw -L../../libwripc -lwripc
CFLAGS = -I. -O2 -I../../include -DDEBUG -I../../../kernel/include -I../../wrsw_hal -I../../libwripc -g
OUTPUT = spi_test


all:	$(OBJS)
	make -C ../../libswitchhw
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run:  all
	- scp $(OUTPUT)  root@${T}:/tmp
	- ssh -t root@${T} "LD_LIBRARY_PATH=/wr/lib /tmp/$(OUTPUT)"


install: all
		mkdir -p ../../rootfs_override/root/tests
		cp $(OUTPUT) ../../rootfs_override/root/tests

clean:
	rm -f $(OUTPUT) $(OBJS)