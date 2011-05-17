#Makefile for Hamming Code
CC = gcc
CFLAGS = -g -O2 -lm  -w -DEXE -DDBG  #-Wall
OBJS = hamming.o
OBJS_M = hamming_matrix.o
OUTPUT= hamming
OUTPUT_M = hamming_matrix

all : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUTPUT)

matrix: $(OBJS_M)
	
	$(CC) $(CFLAGS) $(OBJS_M) -o $(OUTPUT_M)

	
%.o : %.c
	${CC} -c $^ $(CFLAGS)

install:

clean:
	rm -f $(OBJS) $(OBJS_M)

