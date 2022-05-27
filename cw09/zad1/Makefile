CC = gcc
CFLAGS = -Wall -lpthread
DEPS = utils.h

.PHONY: all clean

all: main

%: %.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f main