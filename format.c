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

void help() {
    printf("Use format as follows: format <name of file to format>\n");
}

//Method that writes the disk image with the given size
void write_disk(char *file_name, float file_size) {

  long long int total_bytes = file_size*1000000; //convert to bytes
    
    printf("got here\n");
    FILE *disk = fopen(file_name, "wb+"); //open the disk to write
    
    printf("%p\n", disk);

    //compute the number of inodes
    //file_size of disksize right? Yes
    int num_inodes = ceilf((float )file_size/(float) AVERAGEFILESIZE);
    //why is it of long type? should be pretty small?
    long long int num_blocks_for_inodes = ceilf((float) (num_inodes* sizeof(inode))/ (float) BLOCKSIZE);

    //write boot block
    char boot[] = "bootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootboot";
    printf("%s\n", boot);
    fwrite(boot, BLOCKSIZE, 1, disk);

    //write superblock
    superblock *superblock1 = malloc(sizeof(superblock));
    superblock1->size = total_bytes;
    superblock1->data_offset = num_blocks_for_inodes; //this is data region offset
    superblock1->inode_offset = 0;
    superblock1->free_block = 0;
    superblock1->free_inode = 0;
    superblock1->root_dir = 0; //default to first inode being the root
    //What I add this morning
    fwrite(superblock1, sizeof(superblock), 1, disk);
    free(superblock1);

    //write inode region
    //TODO: make sure the inodes are linked into a list!! (done)
    inode *inodes[num_inodes];
    for(int i=0; i<num_inodes; i++) {
        inodes[i] = malloc(sizeof(inode));
        inodes[i]->disk_identifier = 0;
        inodes[i]->parent_inode_index = -1;
        if (i == num_inodes - 1) {
            //make the last inode have a next of -1 to show end of free list
            inodes[i]->next_inode = -1;
        } else {
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
        inodes[i]->inode_index = i;
        inodes[i]->i2block = -1;
        inodes[i]->i3block = -1;
        inodes[i]->last_block_index = -1;

        fwrite(inodes[i], sizeof(inode), 1, disk);
        free(inodes[i]);
    }
    //padding if needed. TODO. Added in the morning
    
    //write data region
    //TODO: make sure that the data blocks are linked into a list!

    //update file size now that everything is done being written

    //done writing, so close the disk
    fclose(disk);
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
    printf("number of arguments: %d\n", argc);
    printf("second argument: %s\n", argv[1]);
    printf("third argument: %s\n", argv[2]);

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
                } else {
                    help();
                }
            }
        }
    }
}

