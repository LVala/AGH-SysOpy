CC = gcc
CFLAGS = -Wall -c
RAPORT = pomiar.txt

all: main_lib main_sys

timing.o: timing.c
	$(CC) $(CFLAGS) -o timing.o  -DRAPORT=\"$(RAPORT)\" timing.c

main_lib.o: main_lib.c
	$(CC) $(CFLAGS) -o main_lib.o main_lib.c

main_sys.o: main_sys.c
	$(CC) $(CFLAGS) -o main_sys.o main_sys.c

# create executable with library functions
main_lib: main_lib.o timing.o
	$(CC) -o main_lib main_lib.o timing.o

# create executable with system functions
main_sys: main_sys.o timing.o
	$(CC) -o main_sys main_sys.o timing.o


# create test files
test_1.txt:
	rm -f test_1.txt
	for i in {1..1000}; do yes "asd asd ASDD  AS" | head -n 2000 >> test_1.txt; yes "   " | head -n 140 >> test_1.txt; done

test_2.txt:
	rm -f test_2.txt
	for i in {1..1000}; do yes "asdas asdasd d" | head -n 2000 >> test_2.txt; yes "   " | head -n 140 >> test_2.txt; done

test_3.txt:
	rm -f test_3.txt
	for i in {1..1000}; do yes "afsdsd " | head -n 22 >> test_3.txt; yes "   " | head -n 233 >> test_3.txt; done

# tests
test_lib: main_lib test_1.txt test_2.txt test_3.txt
	echo "LIBRARY FUNCTIONS" >> $(RAPORT)
	./main_lib test_1.txt test_lib_output_1.txt
	./main_lib test_2.txt test_lib_output_2.txt
	./main_lib test_3.txt test_lib_output_3.txt

test_sys: main_sys test_1.txt test_2.txt test_3.txt
	echo "SYSTEM FUNCTIONS" >> $(RAPORT)
	./main_sys test_1.txt test_sys_output_1.txt
	./main_sys test_2.txt test_sys_output_2.txt
	./main_sys test_3.txt test_sys_output_3.txt

test:
	make test_lib
	make test_sys
	make clean


clean:
	rm -f *.o main_lib main_sys test_*.txt
