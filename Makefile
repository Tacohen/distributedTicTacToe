CC=gcc
CFLAGS=-g -Wall

all: ttt TTT
	
ttt: ttt.o
	$(CC) -o ttt ttt.o
TTT: TTT.o
	$(CC) -o TTT TTT.o
clean:
	rm -f *.o ttt TTT

