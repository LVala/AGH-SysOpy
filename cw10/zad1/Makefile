CC = gcc
CFLAGS = -Wall
LIBS = -lpthread
DEPS = utils.h

.PHONY: all clean

all: server client

%: %.c $(DEPS)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $<

clean:
	rm -f main