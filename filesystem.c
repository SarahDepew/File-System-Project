//
// Created by Sarah Depew on 4/25/18.
//

#include "filesystem.h"
#include "boolean.h"

//global variables
file_table_entry fileTable[FILETABLESIZE];

void setup() { //sets the global variable for the root and the global disk pointer :) (have a table of file ptrs that are disks that are currently mounted)

}

/* f_open() method */ //TODO: assume this is the absolute file path
int f_open(char* filepath, int access, permission_value permissions) {
    //obtain the proper inode on the file //TODO: ask dianna how to do this? (page 326)

    //read the inode into memory
    FILE *disk = fopen("Disk Image", "r+");

}