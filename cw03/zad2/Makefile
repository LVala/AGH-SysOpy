CC = gcc
CFLAGS = -Wall
RAPORT = raport.txt

all: main

main: main.c
	$(CC) $(CFLAGS) -o main -DRAPORT=\"$(RAPORT)\" main.c

test_1: main
	./main 0.00005 5

test_2: main
	./main 0.00005 20

test_3: main
	./main 0.000003 15

test_4: main
	./main 0.000003 70

test_5: main
	./main 0.0000001 15

test_6: main
	./main 0.0000001 70

test:
	make test_1
	make test_2
	make test_3
	make test_4
	make test_5
	make test_6
	make clean

clean:
	rm -f *.o main w*.txt