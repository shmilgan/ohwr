CC=gcc
OBJS = gen_pll_coefs.o
LDFLAGS = 
CFLAGS = 
OUTPUT = gen_pll_coefs

all:	$(OBJS)
	${CC} -o $(OUTPUT) $(OBJS) $(LDFLAGS)

%.o:	%.c
	${CC} -c $^ $(CFLAGS)
