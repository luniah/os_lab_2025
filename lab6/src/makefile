CC = gcc
CFLAGS = -pthread

OBJS = mult_modulo.o

all: client server

mult_modulo.o: mult_modulo.c mult_modulo.h
	$(CC) $(CFLAGS) -c mult_modulo.c

client: client.c $(OBJS)
	$(CC) $(CFLAGS) -o client client.c $(OBJS)

server: server.c $(OBJS)
	$(CC) $(CFLAGS) -o server server.c $(OBJS)

clean:
	rm -f client server $(OBJS)
	