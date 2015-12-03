CC=gcc
CFLAGS=-Wall -g
LFLAGS= -lpthread
PROGRAM=sv cl test-ping test-pingd test-pingc
OBJS=test-sv.o test-cl.o test-ping.o test-pingd.o test-pingc.o nstdio.o
RM=rm

all: sv cl test-ping test-pingd test-pingc

cl: test-cl.o nstdio.o
	$(CC) $(LFLAGS) -o cl test-cl.o nstdio.o

sv: test-sv.o nstdio.o
	$(CC) $(LFLAGS) -o sv test-sv.o nstdio.o

test-ping: test-ping.o nstdio.o
	$(CC) $(LFLAGS) -o test-ping test-ping.o nstdio.o

test-pingc: test-pingc.o nstdio.o
	$(CC) $(LFLAGS) -o test-pingc test-pingc.o nstdio.o

test-pingd: test-pingd.o nstdio.o
	$(CC) $(LFLAGS) -o test-pingd test-pingd.o nstdio.o

SUFFIX:.c .o 
.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) $(PROGRAM) $(OBJS) *~
