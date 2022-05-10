CC=gcc
CFLAGS=-std=gnu99 -g
OBJS=smtp-server.o pop3-server.o main.o

all: dev-mail

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
dev-mail: $(OBJS)
	gcc -o dev-mail $(OBJS) -L../SockFramework -L../Cute -L. -lsockf -lcute
clean:
	rm $(OBJS) dev-mail
