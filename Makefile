all: format filesystem open_dir formatdir test test_mount_read_write tests parser shell

filesystem:
	gcc -g -ggdb -Wall -fpic -c filesystem.c
	gcc -g -ggdb -Wall -o libfile.so filesystem.o -shared

f_readdir_test: f_readdir_test.c
	gcc -g -ggdb -Wall -o f_readdir_test f_readdir_test.c -L. -lfile -lm

test_mount_read_write: fmount_read_write.c
	gcc -g -ggdb -Wall -o test_mount_read_write fmount_read_write.c -L. -lfile -lm

formatdir: format_dir.c
	gcc -g -ggdb -Wall -o formatdir format_dir.c -lm

format:
	gcc -g -ggdb -c format.c
	gcc -g -ggdb -o format format.o -lm

open_dir: test_open_dir.c
	gcc -g -ggdb -Wall -o open_dir test_open_dir.c -L. -lfile -lm

test: test.c
	gcc -g -ggdb -Wall -o test test.c -L. -lfile -lm

tests: tests.c
	gcc -g -ggdb -Wall -o tests tests.c -L. -lfile -lm

shell: parser.o boolean.h builtins.h
	gcc -c -Wall -c shell.c -lreadline
	gcc -g -ggdb -Wall -o shell shell.o parser.o -L. -lfile -lm -lreadline

parser: parser.c parser.h boolean.h builtins.h
	gcc -c -Wall -c parser.c -lreadline

clean:
	rm *.o format libfile.so test open_dir formatdir test_mount_read_write f_readdir_test tests shell parser
