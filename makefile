all: mainA mainB

mainA: mainA.c
	gcc -g -pthread -Wall mainA.c -o mainA

mainB: mainB.c
	gcc -g -pthread -Wall mainB.c -o mainB

clean:
	rm -f mainA mainB
