all: format filesystem formatdir test test_mount_read_write

test_mount_read_write: fmount_read_write.c
	gcc -g -ggdb -o test_mount_read_write fmount_read_write.c -lfile

formatdir: format_dir.c
	gcc -g -ggdb -o formatdir format_dir.c -lm

format: format.c
	gcc -g -ggdb -o format format.c -lm

test: test.c
	gcc -g -ggdb -o test test.c -L. -lfile

filesystem:
	gcc -g -ggdb -Wall -fpic -c filesystem.c
	gcc -g -ggdb -o libfile.so filesystem.o -shared

clean:
	rm *.o format libfile.so
