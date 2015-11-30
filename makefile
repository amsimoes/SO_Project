main: main.o memoria.o func.o
	gcc -o main -Wall -pthread -D_REENTRANT main.o func.o memoria.o

main.o: main.c main.h memoria.h func.h
	gcc -c main.c

memoria.o: memoria.c func.h
	gcc -c memoria.c

func.o: func.c
	gcc -c func.c