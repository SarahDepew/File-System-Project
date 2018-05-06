#include <stdio.h>
#include "tests.h"
#include "filesystem.h"

#define CORRECTNUMARGUMENTS 2
#define FILELOCATION 1

void help() {
    printf("Use tests as follows: tests <name of disk to test>\n");
}

//Pass in the name of the disk to be tested
int main(int argc, char *argv[]) {
    if(argc != CORRECTNUMARGUMENTS) {
        help();
    } else {
        run_tests(argv[FILELOCATION]);
    }
}

void run_tests(char *disk_to_test) {
    printf("*************Testing f_open*************\n");
    

    printf("*************Done Testing f_open*************\n");
}

//1) test fopen on a file that does not exist with a valid path and writing/appending as the value - expected behavior is that the file is created and the file is added to the directory as well as opened in the file table
