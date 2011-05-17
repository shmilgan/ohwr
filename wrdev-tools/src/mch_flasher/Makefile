CC=gcc
OBJS=mch_flasher.o serial_linux.o
OUTPUT=../../mch_flasher

all:	$(OBJS)
		${CC} -o $(OUTPUT) $(OBJS)

clean:
		rm -f $(OBJS) $(OUTPUT)