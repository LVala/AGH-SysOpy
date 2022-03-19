CC = gcc
CFLAGS = -Wall

all: main_dir main_nftw

main_dir: main_dir.c
	$(CC) $(CFLAGS) -o main_dir main_dir.c

main_nftw: main_nftw.c
	$(CC) $(CFLAGS) -o main_nftw main_nftw.c

clean:
	rm -f main_dir main_nftw