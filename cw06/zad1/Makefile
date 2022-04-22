CC = gcc
CFLAGS = -Wall

all: client server

client: client.c
	$(CC) $(CFLAGS) -o client client.c

server: server.c
	$(CC) $(CLFAGS) -o server server.c

clean:
	rm -f client server
