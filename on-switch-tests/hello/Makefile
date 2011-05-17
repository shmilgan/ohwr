CC=$(CROSS_COMPILE)gcc
OBJS = hello.o
LDFLAGS = 
CFLAGS = -I. -O2 -I../../include -DDEBUG
OUTPUT = hello

all:	$(OBJS)
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run::	all
	scp $(OUTPUT) root@$(T):/tmp
	- ssh -t root@$(T) "/tmp/$(OUTPUT)"

clean:
	rm -f $(OUTPUT) $(OBJS)