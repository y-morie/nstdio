CC=gcc
CFLAGS=-Wall -fPIC -shared
LFLAGS= -lpthread 
OBJS=nstdio.o
SOBJS=libnstdio.so
HFILE=nstdio.h
INC=../include
LIB=../lib
RM=rm
CP=cp

all: ${SOBJS}

libnstdio.so: ${OBJS}
	$(CC) $(CFLAGS) -o ${SOBJS} ${OBJS}

SUFFIX:.c .o 
.c.o:
	$(CC) -c $(CFLAGS) $<

install:
	$(CP) ${SOBJS} $(LIB)
	$(CP) ${HFILE} $(INC)

clean:
	-$(RM) $(OBJS)
	-$(RM) $(SOBJS)
	-$(RM) *~

uninstall:
	-$(RM} ${LIB}$(SOBJS)
	-$(RM} ${INC}$(HFILE)
