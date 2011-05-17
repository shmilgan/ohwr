CC=$(CROSS_COMPILE)gcc
OBJS = cpu_io.o mb_io.o mblaster.o main.o
CFLAGS = -I. -O3
OUTPUT = fpgaboot


all:	$(OBJS)
		${CC} -s -o $(OUTPUT) $(OBJS)
		
%.o:	%.c
		${CC} -c $^ $(CFLAGS)

install-image:
		cp $(OUTPUT) ../root/usr/sbin

install:	all
		scp $(OUTPUT) root@192.168.1.2:/root
	
clean:
	  rm -f $(OUTPUT) $(OBJS)