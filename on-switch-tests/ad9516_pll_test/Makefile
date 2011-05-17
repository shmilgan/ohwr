CC=$(CROSS_COMPILE)gcc
OBJS = ad9516_test.o
LDFLAGS = -lswitchhw -L../../libswitchhw
CFLAGS = -I. -O2 -I../../include -DDEBUG
OUTPUT = ad9516_test
EXTRAFILES = regs.txt

all:	$(OBJS)
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run::	all
	scp $(OUTPUT) $(EXTRAFILES) root@$(T):/tmp
	- ssh -t root@$(T) "/tmp/$(OUTPUT)"

clean:
	rm -f $(OUTPUT) $(OBJS)