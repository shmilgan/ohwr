CC=$(CROSS_COMPILE)gcc
OBJS = minic_test.o term.o gigaspy.o
LDFLAGS = -lswitchhw -L../../libswitchhw
CFLAGS = -I. -O2 -I../../include -DDEBUG -I../../../kernel/include
OUTPUT = minic_test


all:	$(OBJS)
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)
	${CC} -o timestamping timestamping.c $(CFLAGS)

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