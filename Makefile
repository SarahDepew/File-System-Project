all: opentest formattest

formattest: format.c
	gcc -o formattest format.c

opentest: fopenstart.c filesystem.h filesystem.o
	gcc -o opentest fopenstart.c filesystem.h

filesystem.o: filesystem.c filesystem.h boolean.h
	gcc -c filesystem.c filesystem.h boolean.h

clean:
	rm *.o
