
OBJS = mch_flasher.o serial_linux.o
FLASHER = mch_flasher

all: $(FLASHER)

$(FLASHER): $(OBJS)
	${CC} -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(FLASHER) *~
