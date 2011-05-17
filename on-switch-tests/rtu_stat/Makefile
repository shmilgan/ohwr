include ../../../Makedefs

CC=$(CROSS_COMPILE_ARM)gcc
OBJS = rtu_stat.o
LDFLAGS = -lswitchhw -L../../libswitchhw -L../../libwripc -L../../libptpnetif -lwripc -lptpnetif 
CFLAGS = -I. -O2 -I../../include -DDEBUG -I../../../kernel/include -I../../libptpnetif -I../../wrsw_hal -I../../libwripc -I../../wrsw_rtud -g 
OUTPUT = rtu_stat


all:	$(OBJS)
	make -C ../../libswitchhw
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run:  all
	- scp $(OUTPUT)  root@192.168.1.16:/tmp
	- scp $(OUTPUT)  root@192.168.1.13:/tmp


install: all
		mkdir -p ../../rootfs_override/root/tests
		cp $(OUTPUT) ../../rootfs_override/root/tests

clean:
	rm -f $(OUTPUT) $(OBJS)
	
deploy: all
		scp $(OUTPUT) $(WR_INSTALL_ROOT)/bin