CC = gcc
CFLAGS = -c -Wall
MEANINGFUL_MESSAGE = systemy operacyjne to fajny przedmiot
RAPORT = raport.txt

all: main

# create executable
mylib.o: mylib.c
	$(CC) $(CFLAGS) -o mylib.o mylib.c

libmylib.a: mylib.o
	ar cr libmylib.a mylib.o

main.o: main.c
	$(CC) $(CFLAGS) -o main.o -DRAPORT=\"$(RAPORT)\" main.c

main: main.o libmylib.a
	$(CC) main.o -o main -L. -lmylib

# create test files
b_test.txt:
	yes $(MEANINGFUL_MESSAGE) | head -c 10000000 > b_test.txt
m_test.txt:
	yes $(MEANINGFUL_MESSAGE) | head -c 1000000 > m_test.txt
s_test.txt:
	yes $(MEANINGFUL_MESSAGE) | head -c 100000 > s_test.txt

# tests
test_few_small: main b_test.txt m_test.txt s_test.txt
	echo "SAVE TO MEMORY TEST - FEW SMALL FILES" >> $(RAPORT)
	./main create_table 3 wc_files `for i in {1..3}; do echo -n 's_test.txt '; done`

test_many_small: main b_test.txt m_test.txt s_test.txt
	echo "SAVE TO MEMORY TEST - MANY SMALL FILES" >> $(RAPORT)
	./main create_table 10 wc_files `for i in {1..10}; do echo -n 's_test.txt '; done`

test_few_medium: main b_test.txt m_test.txt s_test.txt
	echo "SAVE TO MEMORY TEST - FEW MEDIUM FILES" >> $(RAPORT)
	./main create_table 3 wc_files `for i in {1..3}; do echo -n 'm_test.txt '; done`

test_many_medium: main b_test.txt m_test.txt s_test.txt
	echo "SAVE TO MEMORY TEST - MANY MEDIUM FILES" >> $(RAPORT)
	./main create_table 10 wc_files `for i in {1..10}; do echo -n 'm_test.txt '; done`

test_few_big: main b_test.txt m_test.txt s_test.txt
	echo "SAVE TO MEMORY TEST - FEW BIG FILES" >> $(RAPORT)
	./main create_table 3 wc_files `for i in {1..3}; do echo -n 'b_test.txt '; done`

test_many_big: main b_test.txt m_test.txt s_test.txt
	echo "SAVE TO MEMORY TEST - MANY BIG FILES" >> $(RAPORT)
	./main create_table 10 wc_files `for i in {1..10}; do echo -n 'b_test.txt '; done`

test_free_blocks: main b_test.txt m_test.txt s_test.txt
	echo "FREE BLOCKS TEST" >> $(RAPORT)
	./main create_table 10 `for i in {-..10}; do echo -n 'wc_files m_test.txt '; done`\
	`for i in {0..9}; do echo -n "remove_block $$i "; done`

test_make_and_free_blocks: main b_test.txt m_test.txt s_test.txt
	echo "CREATE AND FREE BLOCKS TEST" >> $(RAPORT)
	./main create_table 10 `for i in {1..10}; do echo -n 'wc_files m_test.txt remove_block 0 '; done`

test:
	make test_few_small
	make test_many_small
	make test_few_medium
	make test_many_medium
	make test_few_big
	make test_many_big
	make test_free_blocks
	make test_make_and_free_blocks
	make clean

clean:
	rm -f *.o *.a main errorlog *_test.txt