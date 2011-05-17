include ../../../Makedefs

CC=$(CROSS_COMPILE_ARM)gcc
OBJS = wr_mon.o term.o 
LDFLAGS = -lswitchhw -L../../libswitchhw -L../../libwripc -L../../libptpnetif -lwripc -lptpnetif
CFLAGS = -I. -O2 -I../../include -DDEBUG -I../../../kernel/include -I../../libptpnetif -I../../PTPWRd -I../../wrsw_hal -I../../libwripc -g
OUTPUT = wr_mon

all:	$(OBJS)
	make -C ../../libswitchhw
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run:  all
	- scp $(OUTPUT)  root@192.168.1.16:/tmp

deploy:
		cp $(OUTPUT) ../../rootfs_override/wr/bin


clean:
	rm -f $(OUTPUT) $(OBJS)