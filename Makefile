all: format filesystem test 

format: format.c
	gcc -g -ggdb -o format format.c -lm

test: test.c
	gcc -g -ggdb -o test test.c -L. -lfile

filesystem:
	gcc -g -ggdb -Wall -fpic -c filesystem.c
	gcc -g -ggdb -o libfile.so filesystem.o -shared

clean:
	rm *.o
