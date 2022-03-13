CC = gcc
CFLAGS = -c -Wall

all: static shared

staticmylib.o: mylib.c
	$(CC) $(CFLAGS) -o staticmylib.o mylib.c

static: staticmylib.o
	ar cr libmylib.a staticmylib.o

sharedmylib.o: mylib.c
	$(CC) $(CFLAGS) -fPIC -o sharedmylib.o mylib.c

shared: sharedmylib.o
	$(CC) -shared -fPIC -o libmylib.so sharedmylib.o

clean:
	rm *.o *.a *.so
