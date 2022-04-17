CC = gcc
CFLAGS = -Wall

all: producer consumer runtest

producer: producer.c
	$(CC) $(CFLAGS) -o producer producer.c

consumer: consumer.c
	$(CC) $(CFLAGS) -o consumer consumer.c

runtest: runtest.c
	$(CC) $(CFLAGS) -o runtest runtest.c

test: runtest producer consumer
	./runtest 5
	./runtest 10
	./runtest 5000
	make clean

clean:
	rm -f producer consumer runtest *.fifo

cleantest:
	rm -f *_out.txt