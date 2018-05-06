#include "filesystem.h"

//Tests for f_open
/*
1) test fopen on a file that does not exist with a valid path and writing/appending as the value - expected behavior is that the file is created and the file is added to the directory as well as opened in the file table
2) test fopen on a file that exists - expected behavior is that the file is opened with the certian access and permissions values and inserted in the file table
3) test fopen on a file that does not exist and that has an invalid path name - expected behavior is returning an invalid file descriptor and an error
4) test fopen on a file that does not exist and trying to read it - expected behavior is that the file is not created and an error value is returned
*/


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

*/

//Tests for f_close
/*
1) Check that the
*/













//Helper Methods
void help();
void run_tests(char *disk_to_test); 
