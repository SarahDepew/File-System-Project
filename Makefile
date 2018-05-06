all: format filesystem open_dir formatdir test test_mount_read_write

filesystem:
	gcc -g -ggdb -Wall -fpic -c filesystem.c
	gcc -g -ggdb -o libfile.so filesystem.o -shared

f_readdir_test: f_readdir_test.c
	gcc -g -ggdb -o f_readdir_test f_readdir_test.c -L. -lfile -lm

test_mount_read_write: fmount_read_write.c
	gcc -g -ggdb -o test_mount_read_write fmount_read_write.c -L. -lfile -lm

formatdir: format_dir.c
	gcc -g -ggdb -o formatdir format_dir.c -lm

format: format.c
	gcc -g -ggdb -o format format.c -lm

open_dir: test_open_dir.c
	gcc -g -ggdb -o open_dir test_open_dir.c -L. -lfile -lm

test: test.c
	gcc -g -ggdb -o test test.c -L. -lfile -lm

clean:
	rm *.o format libfile.so test open_dir formatdir test_mount_read_write f_readdir_test
