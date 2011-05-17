CC=gcc
OBJS=uboot_env.o
OUTPUT=../../uboot_env
LDFLAGS = -lz

all:	$(OBJS)
		${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

clean:
		rm -f $(OBJS) $(OUTPUT)