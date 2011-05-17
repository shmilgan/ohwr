CC=gcc
OBJS=gen_fpga_image.o minilzo.o md5.o
OUTPUT=../../gen_fpga_image
LDFLAGS=

all:	$(OBJS)
		${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

clean:
		rm -f $(OBJS) $(OUTPUT)