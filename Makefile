all: format open_dir filesystem formatdir test test_mount_read_write

test_mount_read_write: fmount_read_write.c
	gcc -g -ggdb -o test_mount_read_write fmount_read_write.c -L. -lfile

formatdir: format_dir.c
	gcc -g -ggdb -o formatdir format_dir.c -lm

format: format.c
	gcc -g -ggdb -o format format.c -lm

open_dir: test_open_dir.c
	gcc -g -ggdb -o open_dir test_open_dir.c -L. -lfile -lm

test: test.c
	gcc -g -ggdb -o test test.c -L. -lfile -lm

filesystem:
	gcc -g -ggdb -Wall -fpic -c filesystem.c
	gcc -g -ggdb -o libfile.so filesystem.o -shared

clean:
	rm *.o format libfile.so
