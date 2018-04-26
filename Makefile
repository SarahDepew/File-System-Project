all: opentest

opentest: fopenstart.c filesystem.h filesystem.o
	gcc -o opentest fopenstart.c filesystem.h filesystem.o

filesystem.o: filesystem.c filesystem.h boolean.h
	gcc -c filesystem.c filesystem.h boolean.h
