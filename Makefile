CC=gcc
CFLAGS=-std=gnu99 -g
OBJS=

all: dev-smtp dev-pop3

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
dev-smtp: $(OBJS) smtp-server.o
	gcc -o dev-smtp smtp-server.o -L../SockFramework -L../Cute -L. -lsockf -lcute
dev-pop3: $(OBJS) pop3-server.o
	gcc -o dev-pop3 pop3-server.o -L../SockFramework -L../Cute -L. -lsockf -lcute
clean:
	rm $(OBJS) dev-smtp dev-pop3 *.o
