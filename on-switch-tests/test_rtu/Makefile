CC=$(CROSS_COMPILE)gcc
OBJS = rtu_sim.o rtu_hw.o rtu_test.o rtu_common.o rtu_test_data.o rtu_test_main.o 
LDFLAGS = -lswitchhw -L../../libswitchhw 
CFLAGS = -I. -O2 -I../../include -DDEBUG
OUTPUT = rtu_sim

all:	$(OBJS)
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)

run::	all
	scp $(OUTPUT) root@$(T):/tmp
	- ssh -t root@$(T) "/tmp/$(OUTPUT)"

clean:
	rm -f $(OUTPUT) $(OBJS)