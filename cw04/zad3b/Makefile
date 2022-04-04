CC = gcc
CFLAGS = -Wall

all: sender catcher

sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

catcher: catcher.c
	$(CC) $(CFLAGS) -o catcher catcher.c

clean:
	rm -f sender catcher
