CC = gcc
CFLAGS = -Wall


all: main_fork main_exec

main_fork: main.c
	$(CC) $(CFLAGS) -o main_fork -DFORK main.c

main_exec: main.c
	$(CC) $(CFLAGS) -o main_exec -DEXEC main.c

clean:
	rm -f *.o main_fork main_exec
