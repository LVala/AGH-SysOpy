CC = gcc
CFLAGS = -Wall -c
RAPORT = pomiar_zad_1.txt

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
test_input.txt:
	rm -f test.txt
	for i in {1..1000}; do yes "asd asd ASDD  AS" | head -n 200 >> test_input.txt; yes "   " | head -n 140 >> test_input.txt; done



# tests
test_lib: main_lib test_input.txt
	echo "LIBRARY FUNCTIONS" >> $(RAPORT)
	./main_lib test_input.txt test_lib_output.txt

test_sys: main_sys test_input.txt
	echo "SYSTEM FUNCTIONS" >> $(RAPORT)
	./main_sys test_input.txt test_sys_output.txt

test:
	make test_lib
	make test_sys
	make clean



clean:
	rm -f *.o main_lib main_sys test_*.txt
