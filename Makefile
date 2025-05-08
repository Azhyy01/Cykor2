CC = gcc
CFLAGS = -Wall -g

source: source.o
	gcc -o source source.o -g

source.o: source.c
	gcc -c source.c -g

clean:
	rm source.o source
