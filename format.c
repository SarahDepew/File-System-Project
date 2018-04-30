//
// Created by Sarah Depew on 4/27/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <memory.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "boolean.h"
#include "filesystem.h"

#define MINNUMARGUMENTS 2
#define MAXNUMARGUMENTS 3
#define FLAGLOCATION 1
#define FILELOCATION 2
#define FILELOCATIONBASIC 1
#define DEFAULTFILESIZE 1
#define AVERAGEFILESIZE 0.02
#define BLOCKSIZE 512

char *flag = "-s";
char *hash = "#";
char *deliminator = " ";
FILE *disk;

void help() {
    printf("Use format as follows: format <name of file to format>\n");
}

void open_disk(char *file_name) {
    disk = fopen(file_name, "wb+"); //open the disk to write
}

void close_disk() {
    fclose(disk);
}

void write_boot_block() {
    //string has a null at the end so should add 1 to the size //TODO: I don't think this matters...
    char *boot = malloc(SIZEOFBOOTBLOCK * sizeof(char) + 1);
    memset(boot, 0, SIZEOFBOOTBLOCK + 1);
    strcpy(boot,
           "bootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootboot");
    fwrite(boot, SIZEOFBOOTBLOCK, 1, disk);
    free(boot);
}

void write_padding(int amount_padding) {
    char *padding = malloc(amount_padding * sizeof(char));
    for (int i = 0; i < amount_padding; i++) {
        padding[i] = ' ';
    }

    fwrite((void *) padding, amount_padding, 1, disk);
    free(padding);
}

void write_super_block(int data_offset) {
    //write the superblock
    superblock *superblock1 = malloc(sizeof(superblock));
    memset(superblock1, 0, sizeof(superblock));
    superblock1->size = BLOCKSIZE;
    superblock1->data_offset = data_offset; //this is data region offset
    superblock1->inode_offset = 0;
    superblock1->free_block = 1; //the first block is the root directory
    superblock1->free_inode = 1; //the first inode is the root directory
    superblock1->root_dir = 0; //default to first inode being the root directory
    fwrite(superblock1, sizeof(superblock), 1, disk);
    free(superblock1);

    //write the remaining bytes at the end of the file
    int bytes_remaining_superblock = SIZEOFSUPERBLOCK - sizeof(superblock);
    printf("bytes remaining: %d\n", bytes_remaining_superblock);
    write_padding(bytes_remaining_superblock);
}

void write_inode_region(int num_inodes, int num_blocks_inodes) {
    //write inode region
    inode *inodes[num_inodes];
    for (int i = 0; i < num_inodes; i++) {
        inodes[i] = (inode *) malloc(sizeof(inode));
        memset(inodes[i], 0, sizeof(inode));
        inodes[i]->disk_identifier = 0;
        inodes[i]->parent_inode_index = -1;
        if (i == 0) {
            // ROSE: this should be the root_dir node
            inodes[i]->parent_inode_index = -1;
            //pointing to the start of the data region
            inodes[i]->dblocks[0] = 0; //the first block is the root directory
            inodes[i]->next_inode = -1; //unused in occupied inodes

            inodes[i]->size = 2* sizeof(directory_entry); //this is how we can tell where we are in the block to continue writing to...
            inodes[i]->uid = 0;
            inodes[i]->gid = 0;
            //TODO: modify these values, since this is when directory is created...
            inodes[i]->ctime = 0;
            inodes[i]->mtime = 0;
            inodes[i]->atime = 0;
            inodes[i]->type = DIR;
            inodes[i]->permission = 0;
            inodes[i]->inode_index = i; //this is the only value that won't be replaced
            inodes[i]->i2block = -1;
            inodes[i]->i3block = -1;
            inodes[i]->last_block_index = 0; //since the only block is this block...

            fwrite(inodes[i], sizeof(inode), 1, disk);
            free(inodes[i]);
            continue; //want to skip the values, below
        }

        if (i == num_inodes - 1) {
            //make the last inode have a next of -1 to show end of free list
            inodes[i]->next_inode = -1;
        } else {
            //ROSE: I don't think this is right
            inodes[i]->next_inode = i + 1;
        }
        inodes[i]->size = 0;
        inodes[i]->uid = 0;
        inodes[i]->gid = 0;
        inodes[i]->ctime = 0;
        inodes[i]->mtime = 0;
        inodes[i]->atime = 0;
        inodes[i]->type = 0;
        inodes[i]->permission = 0;
        inodes[i]->inode_index = i; //this is the only value that won't be replaced
        inodes[i]->i2block = -1;
        inodes[i]->i3block = -1;
        inodes[i]->last_block_index = -1;

        fwrite(inodes[i], sizeof(inode), 1, disk);
        free(inodes[i]);
    }

    //padding if needed.
    long padding_value =
            num_blocks_inodes * BLOCKSIZE - num_inodes * sizeof(inode); //total bytes minus those taken by inodes
    printf("number of inodes: %d\n", num_inodes);
    printf("number of bytes needed for inodes: %d\n", num_blocks_inodes * BLOCKSIZE);
    printf("size of inode: %lu\n", sizeof(inode));
    printf("number of bytes for the inodes %lu\n", num_inodes * sizeof(inode));
    write_padding(padding_value);
}

void write_data_region(long total_bytes, int num_blocks_for_inodes) {
    //write data region
    //TODO: make sure that the data blocks are linked into a list! (unit test this...)
    long bytes_left_for_data_blocks =
            total_bytes - SIZEOFSUPERBLOCK - SIZEOFBOOTBLOCK - num_blocks_for_inodes * BLOCKSIZE;
    long num_data_blocks = ceilf((float) bytes_left_for_data_blocks / (float) BLOCKSIZE);
    printf("num data blocks %ld\n", num_data_blocks);

    for (int j = 0; j < num_data_blocks; j++) {
        //create the list before writing to the file!
        void *block_to_write = malloc(BLOCKSIZE); //malloc enough memory
        memset(block_to_write, 0, BLOCKSIZE);

        if (j == 0) {
            // the root dir data data_region. ALL TEMP
            directory_entry *directories = malloc(2*sizeof(directory_entry));
            directories[0].inode_index = 0;
            strcpy(directories[0].filename, ".");

            directories[1].inode_index = 0;
            strcpy(directories[1].filename, "..");

            memcpy(block_to_write, directories, sizeof(directory_entry) *2);
        } else if (j == num_data_blocks - 1) {
            ((block *) block_to_write)->next_free_block = -1;
        } else {
            ((block *) block_to_write)->next_free_block = j + 1;
        }

        fwrite(block_to_write, BLOCKSIZE, 1, disk);
        free(block_to_write);
    }
}

//Method that writes the disk image with the given size (filename is name of file and filesize is disk size to generate in mb)
void write_disk(char *file_name, float file_size) {
    long long int total_bytes = file_size * 1000000; //convert mb to bytes

    //open the disk first
    open_disk(file_name);

    //write boot block
    write_boot_block();

    //write superblock
    //compute the number of inodes, so that you have the data region offset
    int num_inodes = ceilf((float) file_size / (float) AVERAGEFILESIZE); //compute the minimum number of inodes
    int num_blocks_for_inodes = ceilf((float) (num_inodes * sizeof(inode)) / (float) BLOCKSIZE); //compute the data blocks needed for this number of inodes
    num_inodes = floor(num_blocks_for_inodes * BLOCKSIZE / sizeof(inode)); //update the number of inodes based on the number of blocks for inodes

    printf("num_inodes: %d\n", num_inodes);
    printf("num blocks for inodes: %d\n", num_blocks_for_inodes);

    write_super_block(num_blocks_for_inodes);

    //write inode region
    write_inode_region(num_inodes, num_blocks_for_inodes);

    //write data region
    write_data_region(total_bytes, num_blocks_for_inodes);

    //done writing, so close the disk
    close_disk();
}

/*
 * Parse the command line input and ensure that the user has put in the correct information
 */
boolean parseCmd(char *integer, long long int *MB) {
    //(error checking gotten from stack overflow)
    const char *nptr = integer;                     /* string to read               */
    char *endptr = NULL;                            /* pointer to additional chars  */
    int base = 10;                                  /* numeric base (default 10)    */
    long long int number = 0;                       /* variable holding return      */

    /* reset errno to 0 before call */
    errno = 0;

    /* call to strtol assigning return to number */
    number = strtoll(nptr, &endptr, base);

    /* test return to number and errno values */
    if (nptr == endptr) {
        printf(" number : %lld  invalid  (no digits found, 0 returned)\n", number);
        return FALSE;
    } else if (errno == ERANGE && number == LONG_MIN) {
        printf(" number : %lld  invalid  (underflow occurred)\n", number);
        return FALSE;
    } else if (errno == ERANGE && number == LONG_MAX) {
        printf(" number : %lld  invalid  (overflow occurred)\n", number);
        return FALSE;
    } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
        printf(" number : %lld  invalid  (base contains unsupported value)\n", number);
        return FALSE;
    } else if (errno != 0 && number == 0) {
        printf(" number : %lld  invalid  (unspecified error occurred)\n", number);
        return FALSE;
    } else if (errno == 0 && nptr && *endptr != 0) {
        printf(" number : %lld    invalid  (since additional characters remain)\n", number);
        return FALSE;
    }

    //passed all the if statements and so can assigned the seconds value as the one input
    *MB = number;
    return TRUE;
}

//Please note that we are assuming a valid input is given here; specifically, we are assuming a valid number is put
int main(int argc, char *argv[]) {
    if (argc != MINNUMARGUMENTS && argc != MAXNUMARGUMENTS) {
        help();
    } else {
        if (argc == MINNUMARGUMENTS) {
            printf("file name: %s\n", argv[FILELOCATIONBASIC]);
            write_disk(argv[FILELOCATIONBASIC], DEFAULTFILESIZE);
        } else {
            char *token = strtok(argv[FLAGLOCATION], deliminator);
            printf("token %s\n", token);

            printf("strcmp value %d\n", strcmp(flag, token));
            if (strcmp(flag, token) == 0) {
                token = strtok(NULL, deliminator);
                printf("token %s\n", token);

                token = strtok(token, hash);
                printf("number of megabytes %s\n", token);

                //convert the token to an integer value
                long long int MB;
                if (parseCmd(token, &MB) == TRUE) {
                    printf("number of megabytes after translation %lld\n", MB);
                    write_disk(argv[FILELOCATION], MB);
                } else {
                    help();
                }
            }
        }
    }
}
