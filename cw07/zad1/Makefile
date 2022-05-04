CC = gcc
CFLAGS = -Wall
DEPS = main.h

.PHONY: all clean

all: main cook deliverer

%: %.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f main cook deliverer