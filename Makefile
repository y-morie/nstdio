CC=gcc
CFLAGS=-Wall
LFLAGS= -lpthread 
PROGRAM=sv cl test-ping test-pingd test-pingc
OBJS=test-sv.o test-cl.o test-ping.o test-pingd.o test-pingc.o
SOBJS=nstdio.so
RM=rm

all: sv cl test-ping test-pingd test-pingc

cl: test-cl.o nstdio.so
	$(CC) $(LFLAGS) -o cl test-cl.o ./lib/nstdio.so

sv: test-sv.o nstdio.so
	$(CC) $(LFLAGS) -o sv test-sv.o ./lib/nstdio.so

test-ping: test-ping.o nstdio.so
	$(CC) $(LFLAGS) -o test-ping test-ping.o ./lib/nstdio.so

test-pingc: test-pingc.o nstdio.so
	$(CC) $(LFLAGS) -o test-pingc test-pingc.o ./lib/nstdio.so

test-pingd: test-pingd.o nstdio.so
	$(CC) $(LFLAGS) -o test-pingd test-pingd.o ./lib/nstdio.so

nstdio.so: nstdio.c
	$(CC) $(LFLAGS) -fPIC -shared -o ./lib/nstdio.so nstdio.c

SUFFIX:.c .o 
.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) -r $(PROGRAM) $(OBJS) *~ ./lib/$(SOBJS) 
