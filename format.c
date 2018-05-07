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
#include <assert.h>
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

void open_disk_test(char *file_name) {
    disk = fopen(file_name, "rb+"); //open the disk to write
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

void check_boot_block() {
    char *boot = malloc(SIZEOFBOOTBLOCK * sizeof(char) + 1);
    memset(boot, 0, SIZEOFBOOTBLOCK + 1);
    fseek(disk, 0, SEEK_SET);
    fread(boot, SIZEOFBOOTBLOCK, 1, disk);
    assert(strcmp(boot, "bootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootboot") == 0);
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
    write_padding(bytes_remaining_superblock);
}

void check_super_block(int data_offset) {
    void *superblock1 = malloc(SIZEOFSUPERBLOCK);
    memset(superblock1, 0, SIZEOFSUPERBLOCK);
    fseek(disk, SIZEOFBOOTBLOCK, SEEK_SET);
    fread(superblock1, SIZEOFSUPERBLOCK, 1, disk);
    superblock *superblockPtr = superblock1;

    assert(superblockPtr->size == BLOCKSIZE);
    assert(superblockPtr->data_offset == data_offset); //this is data region offset
    assert(superblockPtr->inode_offset == 0);
    assert(superblockPtr->free_block == 1); //the first block is the root directory
    assert(superblockPtr->free_inode == 1); //the first inode is the root directory
    assert(superblockPtr->root_dir == 0); //default to first inode being the root directory

    free(superblock1);
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
            inodes[i]->parent_inode_index = 0;
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
            inodes[i]->permission.owner = '\a';
            inodes[i]->permission.group = '\a';
            inodes[i]->permission.others = '\a';
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
            inodes[i]->next_inode = i + 1;
        }
        inodes[i]->size = 0;
        inodes[i]->uid = 0;
        inodes[i]->gid = 0;
        inodes[i]->ctime = 0;
        inodes[i]->mtime = 0;
        inodes[i]->atime = 0;
        inodes[i]->type = -1;
        inodes[i]->permission.owner = '\a';
        inodes[i]->permission.group = '\a';
        inodes[i]->permission.others = '\a';
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
    write_padding(padding_value);
}

void check_inode_region() {
    //read in the superblock to get the data offset and check the number of inodes there...
    void *superblock1 = malloc(SIZEOFSUPERBLOCK);
    memset(superblock1, 0, SIZEOFSUPERBLOCK);
    fseek(disk, SIZEOFBOOTBLOCK, SEEK_SET);
    fread(superblock1, SIZEOFSUPERBLOCK, 1, disk);
    superblock *superblockPtr = superblock1;
    int data_offset = superblockPtr->data_offset;
    free(superblock1);

    int num_inodes = data_offset*BLOCKSIZE/sizeof(inode);
    inode *currentInode = NULL;

    for (int i = 0; i < num_inodes; i++) {
        currentInode = malloc(sizeof(inode));
        memset(currentInode, 0, sizeof(inode));

        //read in the inode
        fseek(disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + i*sizeof(inode), SEEK_SET);
        fread(currentInode, sizeof(inode), 1, disk);

        assert(currentInode->disk_identifier == 0);
        assert(currentInode->i2block == -1);
        assert(currentInode->i3block == -1);
        assert(currentInode->inode_index == i); //this is the only value that won't be replaced
        assert(currentInode->permission.owner == '\a');
        assert(currentInode->permission.group == '\a');
        assert(currentInode->permission.others == '\a');

        if (i == 0) {
            assert(currentInode->parent_inode_index == 0); //the root inode is it's own parent :)
            assert(currentInode->dblocks[0] == 0); //the first block is the root directory
            assert(currentInode->next_inode == -1); //unused in occupied inodes
            assert(currentInode->size == 2* sizeof(directory_entry)); //this is how we can tell where we are in the block to continue writing to...
            assert(currentInode->uid == 0);
            assert(currentInode->gid == 0);
            //TODO: modify these values once they're fixed in the above code...
            assert(currentInode->ctime == 0);
            assert(currentInode->mtime == 0);
            assert(currentInode->atime == 0);
            assert(currentInode->type == DIR);
            assert(currentInode->last_block_index == 0); //since the only block is this block...
        } else {
          assert(currentInode->parent_inode_index == -1);
          assert(currentInode->size == 0);
          assert(currentInode->uid == 0);
          assert(currentInode->gid == 0);
          assert(currentInode->ctime == 0);
          assert(currentInode->mtime == 0);
          assert(currentInode->atime == 0);
          assert(currentInode->type == -1);
          assert(currentInode->last_block_index == -1);

          if(i == num_inodes-1) {
            assert(currentInode->next_inode == -1);
          } else {
            assert(currentInode->next_inode == i + 1);
          }
        }

        free(currentInode);
    }
}

void write_data_region(long total_bytes, int num_blocks_for_inodes) {
    //write data region
    //TODO: make sure that the data blocks are linked into a list! (unit test this...)
    long bytes_left_for_data_blocks =
            total_bytes - SIZEOFSUPERBLOCK - SIZEOFBOOTBLOCK - num_blocks_for_inodes * BLOCKSIZE;
    long num_data_blocks = ceilf((float) bytes_left_for_data_blocks / (float) BLOCKSIZE);

    for (int j = 0; j < num_data_blocks; j++) {
        //create the list before writing to the file!
        void *block_to_write = malloc(BLOCKSIZE); //malloc enough memory
        memset(block_to_write, 0, BLOCKSIZE);

        if (j == 0) {
            directory_entry *directory_entry_array = block_to_write;
            directory_entry_array[0].inode_index = 0;
            directory_entry_array[1].inode_index = 0;
            strcpy(directory_entry_array[0].filename, ".");
            strcpy(directory_entry_array[1].filename, "..");
        } else if (j == num_data_blocks - 1) {
            ((block *) block_to_write)->next_free_block = -1;
        } else {
            ((block *) block_to_write)->next_free_block = j + 1;
        }
        fwrite(block_to_write, BLOCKSIZE, 1, disk);
        free(block_to_write);
    }
}

void check_data_region(long total_bytes, int num_blocks_for_inodes, int data_region_offset) {
    long bytes_left_for_data_blocks =
            total_bytes - SIZEOFSUPERBLOCK - SIZEOFBOOTBLOCK - num_blocks_for_inodes * BLOCKSIZE;
    long num_data_blocks = ceilf((float) bytes_left_for_data_blocks / (float) BLOCKSIZE);

    //we know that the head of the free list is 1, since the superblock has already been checked
    for (int j = 0; j < num_data_blocks; j++) {
        //create the list before writing to the file!
        void *block_to_read = malloc(BLOCKSIZE); //malloc enough memory
        memset(block_to_read, 0, BLOCKSIZE);

        fseek(disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + data_region_offset*BLOCKSIZE + j*BLOCKSIZE, SEEK_SET);
        fread(block_to_read, BLOCKSIZE, 1, disk);

        if (j == 0) {
            directory_entry *directory_entry_array = block_to_read;
            assert(directory_entry_array[0].inode_index == 0);
            assert(directory_entry_array[1].inode_index == 0);
            assert(strcmp(directory_entry_array[0].filename, ".") == 0);
            assert(strcmp(directory_entry_array[1].filename, "..") == 0);
        } else if (j == num_data_blocks - 1) {
            assert(((block *) block_to_read)->next_free_block == -1);
        } else {
            assert(((block *) block_to_read)->next_free_block == j + 1);
        }

        free(block_to_read);
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
    int num_blocks_for_inodes = ceilf((float) (num_inodes * sizeof(inode)) /
                                      (float) BLOCKSIZE); //compute the data blocks needed for this number of inodes
    num_inodes = floor(num_blocks_for_inodes * BLOCKSIZE /
                       sizeof(inode)); //update the number of inodes based on the number of blocks for inodes

    write_super_block(num_blocks_for_inodes);

    //write inode region
    write_inode_region(num_inodes, num_blocks_for_inodes);

    //write data region
    write_data_region(total_bytes, num_blocks_for_inodes);

    //done writing, so close the disk
    close_disk();
}

void test_disk(char *file_name, float file_size) {
  long long int total_bytes = file_size * 1000000; //convert mb to bytes

  //open the disk first
  open_disk_test(file_name);

  //write boot block
  check_boot_block();

  // //write superblock
  //compute the number of inodes, so that you have the data region offset
  int num_inodes = ceilf((float) file_size / (float) AVERAGEFILESIZE); //compute the minimum number of inodes
  int num_blocks_for_inodes = ceilf((float) (num_inodes * sizeof(inode)) /
                                    (float) BLOCKSIZE); //compute the data blocks needed for this number of inodes
  num_inodes = floor(num_blocks_for_inodes * BLOCKSIZE /
                     sizeof(inode)); //update the number of inodes based on the number of blocks for inodes

  check_super_block(num_blocks_for_inodes);

  //write inode region
  check_inode_region();

  //write data region
  check_data_region(total_bytes, num_blocks_for_inodes, num_blocks_for_inodes);

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
            write_disk(argv[FILELOCATIONBASIC], DEFAULTFILESIZE);
            printf("Disk has been written.\n");
            test_disk(argv[FILELOCATIONBASIC], DEFAULTFILESIZE);
            printf("Disk has been tested.\n");
            printf("Disk may now be used.\n");
        } else {
            char *token = strtok(argv[FLAGLOCATION], deliminator);
            if (strcmp(flag, token) == 0) {
                token = strtok(NULL, deliminator);
                token = strtok(token, hash);

                //convert the token to an integer value
                long long int MB;
                if (parseCmd(token, &MB) == TRUE) {
                    write_disk(argv[FILELOCATION], MB);
                    printf("Disk has been written.\n");
                    test_disk(argv[FILELOCATION], MB);
                    printf("Disk has been tested.\n");
                    printf("Disk may now be used.\n");
                } else {
                    help();
                }
            }
        }
    }
}
