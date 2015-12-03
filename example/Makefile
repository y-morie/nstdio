CC=gcc
PROGRAM=sv cl test-ping test-pingd test-pingc
OBJS=test-sv.o test-cl.o test-ping.o test-pingd.o test-pingc.o
SOBJS=libnstdio.so
LIB=../lib
INC=../include
CFLAGS=-Wall -I$(INC)
LFLAGS=-L$(LIB) -lnstdio -pthread
RM=rm

all: sv cl test-ping test-pingd test-pingc

cl: test-cl.o 
	$(CC) $(LFLAGS) -o cl test-cl.o

sv: test-sv.o 
	$(CC) $(LFLAGS) -o sv test-sv.o

test-ping: test-ping.o
	$(CC) $(LFLAGS) -o test-ping test-ping.o 

test-pingc: test-pingc.o
	$(CC) $(LFLAGS) -o test-pingc test-pingc.o 

test-pingd: test-pingd.o
	$(CC) $(LFLAGS) -o test-pingd test-pingd.o 

SUFFIX:.c .o 
.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) $(PROGRAM)
	$(RM) $(OBJS)
	$(RM)  *~ 
