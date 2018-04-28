all: filesystem test format

format: format.c
	gcc -g -ggdb -o format format.c

test:
	gcc -o -g -ggdb test test.c -L. -lfile

filesystem: filesystem.o
	gcc -g -ggdb -o libfile.so filesystem.o -shared

filesystem.o: filesystem.c filesystem.h boolean.h
	gcc -g -ggdb -Wall -fpic -c filesystem.c

clean:
	rm *.o
