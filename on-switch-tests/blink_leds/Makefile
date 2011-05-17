CC=$(CROSS_COMPILE)gcc
OBJS = blink_leds.o
LDFLAGS = -lswitchhw -L../../libswitchhw
CFLAGS = -I. -O2 -I../../include -DDEBUG
OUTPUT = blink_leds

all:	$(OBJS)
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run:	all
	scp $(OUTPUT) root@$(T):/tmp
	- ssh -t root@$(T) "/tmp/$(OUTPUT)"

install: all
		mkdir -p ../../rootfs_override/root/tests
		cp $(OUTPUT) ../../rootfs_override/root/tests

clean:
	rm -f $(OUTPUT) $(OBJS)