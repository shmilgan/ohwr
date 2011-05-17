CC       = g++
CFLAGS   = -Wall -O2 -g -fPIC -shared
OBJS = hamming.o

all:	$(OBJS) 	
	$(CC) $(CFLAGS) $(OBJS) -o hamming.so

clean: 
	rm -f  *.o *.so

%.o : %.cpp  
	$(CC) $(CFLAGS) -c $^
