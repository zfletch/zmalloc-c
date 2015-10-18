#CC=/opt/local/bin/gcc-mp-4.8
CC=gcc
CFLAGS=-c -Wall -Wextra

all: test

test: test.o zmalloc.o harness.o
	$(CC) test.o zmalloc.o harness.o -o test

test.o: ./test.c
	$(CC) $(CFLAGS) ./test.c

zmalloc.o: ../src/zmalloc.c
	$(CC) $(CFLAGS) ../src/zmalloc.c

harness.o: ./test-harness-c/src/harness.c
	$(CC) $(CFLAGS) ./test-harness-c/src/harness.c

clean:
	rm -rf *o test
