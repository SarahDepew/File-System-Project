#include "filesystem.h"

//Tests for f_open
/*
1) test fopen on a file that does not exist with a valid path and writing/appending as the value - expected behavior is that the file is created and the file is added to the directory as well as opened in the file table
2) test fopen on a file that exists - expected behavior is that the file is opened with the certian access and permissions values and inserted in the file table
3) test fopen on a file that does not exist and that has an invalid path name - expected behavior is returning an invalid file descriptor and an error
4) test fopen on a file that does not exist and trying to read it - expected behavior is that the file is not created and an error value is returned
*/
void test_fopen_create(char *filepath, int access, permission_value *permissions);

//Tests for f_read
/*
1) Read from a file's direct blocks
2) Read from a file's indirect blocks
3) Read from a file's doubly indirect blocks
4) Read from a file's triply indirect blocks
5) Read from the start of the blocks
6) Read from the start of the file
7) Read from the middle of a block in the file to the end of the block in the file
8) Read from the middle of a block in the file to the middle of the next block in the file
9) Try to read more than a file size
10) Try to read past the end of the file...
11) Try to read a file that isn't opened for reading
12) Try negative size or number of times or invalid file descriptor
*/

//Tests for f_write
/*
***compare inodes and superblock and data region***
1)when access == WRITE || READANDWRITE, write small data to the dblocks
2)when access == WRITE || READANDWRITE, write large data to the dblocks (go into idblocks)
3)when access == WRITE || READANDWRITE, write small data to the idblocks
4)when access == WRITE || READANDWRITE, write large data to the idblocks
5)when access == WRITE || READANDWRITE, write small data to the i2blocks
6)when access == WRITE || READANDWRITE, write large data to the i2blocks
7)when access == WRITE || READANDWRITE, write small data to the i3blocks
8)when access == WRITE || READANDWRITE, write large data to the i3blocks
9)when access == WRITE || READANDWRITE, write large enough data to exceed the disk size
10)when access == APPEND, append small data to the dblocks
11)when access == APPEND, append large data to the dblocks
12)when access == APPEND, append small data to the idblocks
13)when access == APPEND, append large data to the idblocks
14)when access == APPEND, append small data to the i2blocks
15)when access == APPEND, append large data to the i2blocks
16)when access == APPEND, append small data to the i3blocks
17)when access == APPEND, append large data to the i3blocks
18)when access == APPEND, append large enough data to exceed the disk size
*/

//Tests for f_close
/*
1) Check that the
*/

//Tests for f_opendir
/*
1)Test a valid path
2)Test an invalid path
3)Test opening root dir only
4)Test opening a directory that is already opened
*/

void test_fopendir_valid(char *disk_to_mount, char* filepath); //1)
void test_fopendir_invalid(char *disk_to_mount, char* filepath); //2)
void test_fopendir_root(char *disk_to_mount, char* filepath); //3)
void test_fopendir_alread_open(char *disk_to_mount, char* filepath); //4)

//Tests for f_readdir
/*
1) Open root and check the root inode's values (should be . and .. at beginning)
2) Open a valid filepath for a directory and check the entries in the directory...use mkdir and an array to confirm (single, double, triple, and direct)
3) Test reading past the end of a file
*/

void test_freaddir_root(char *disk_to_mount); //1)
void test_freaddir_past_end(char *disk_to_mount, char *filepath); //3)

//...


//Tests for f_mount (as of right now, without mounting multiple disks)
/*
1) Check that the values mounted are as expected for the given disk
*/
void test_fmount(char *disk_to_mount);

//Tests for f_unmount (as of right now, without mounting multiple disks)
/*
1) First mount a disk and then check that the spot in the mounted disks array is marked as free and that there are no memory leaks
*/
void test_funmount(char *disk_to_mount); 

//Helper Methods
void help();
void run_tests(char *disk_to_test);
void check_freehead();
