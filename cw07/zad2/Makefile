CC = gcc
CFLAGS = -Wall
DEPS = utils.h

.PHONY: all clean

all: main cook deliverer

%: %.c $(DEPS)
	$(CC) $(CFLAGS) -lpthread -lrt -o $@ $<

clean:
	rm -f main cook deliverer