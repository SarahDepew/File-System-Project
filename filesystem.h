//
// Created by Sarah Depew on 4/25/18.
//

#ifndef HW7_FILESYSTEM_H
#define HW7_FILESYSTEM_H

#include "boolean.h"
#include "stdio.h"
#include "string.h"

#define ERROR -1
#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0
//change this to OPENFILE_MAX?
#define FILETABLESIZE 20
#define FILENAMEMAX 60
#define MOUNTTABLESIZE 20
#define SIZEOFSUPERBLOCK 512
#define SIZEOFBOOTBLOCK 512
#define BLOCKSIZE 512
#define PTRSIZE sizeof(int)
#define DBLOCKS (N_DBLOCKS*superblockPtr->size)
#define IBLOCKS (DBLOCKS + N_IBLOCKS*(superblockPtr->size/PTRSIZE)*superblockPtr->size)
#define I2BLOCKS (IBLOCKS + (superblockPtr->size/PTRSIZE)*(superblockPtr->size/PTRSIZE)*superblockPtr->size)
#define I3BLOCKS (I2BLOCKS + (superblockPtr->size/PTRSIZE)*(superblockPtr->size/PTRSIZE)*(superblockPtr->size/PTRSIZE)*superblockPtr->size)

enum filetype {DIR, REG};
enum fileseek {SSET, SCUR, SEND};
enum permission{SUPER, REGULAR};
enum access{READ, WRITE, READANDWRITE, APPEND};
enum whence{SEEKSET, SEEKCUR, SEEKEND};

typedef struct superblock {
    int size; /* size of blocks in bytes */
    int inode_offset; /* offset of inode region in blocks */
    int data_offset; /* data region offset in blocks */
    int free_inode; /* head of free inode list, index */
    int free_block; /* head of free block list, index */
    int root_dir; /*root directory node index on this disk */
} superblock ;

typedef struct block {
    int next_free_block;
} block;

/* Directory entry struct modeled after V7 */
typedef struct directory_entry {
    int inode_index; //pointer to inode
    char filename[FILENAMEMAX]; //simply the file name and not the absolute path
} directory_entry;

//TODO: see what we actually need here/add a boot block
typedef struct mounted_file_table_entry {
  FILE *disk_ptr;
  char *absolutepath;
} mounted_file_table_entry;

/* file system programs maintains a list of mounted directories information, information as stored as the ‘mounted_disk’ struct */
typedef struct mounted_disk{
    int root_inode; // inode index of the root directory of the disk
    char* mp_name; // mounting point filepath
    int blocksize;
    int inode_region_offset;
    int data_region_offset;
    int free_block;
    int free_inode;
    int root_dir_inode_index;
    void* inode_region;
} mounted_disk;

// structure for f_stat, information all taken from the inode/vnode
typedef struct stat {
    int size; /* number of bytes in file */
    int uid; /* owner’s user ID */
    int gid; /* owner’s group ID */
    int ctime; /* last status change time */
    int mtime; /* last data modification time */
    int atime; /* last access time */
    int type; // dir or regular file
    int permission;
    int inode_index; // the index number for this inode
} stat;

typedef struct permission_value {
    char owner;
    char group;
    char others;
} permission_value;

/* inode struct */
typedef struct inode {
    unsigned int disk_identifier; //identifies the disk (the one that is mounted (for eventual removal))
    int parent_inode_index;
    int next_inode; /* index of next free inode */
    int size; /* number of bytes in file */
    int uid; /* owner’s user ID */
    int gid; /* owner’s group ID */
    int ctime; /* last status change time */
    int mtime; /* last data modification time */
    int atime; /* last access time */
    int type; // dir or regular file
    struct permission_value permission; //file access information
    int inode_index; // the index number for this inode
    int dblocks[N_DBLOCKS]; /* pointers to data blocks */
    int iblocks[N_IBLOCKS]; /* pointers to indirect blocks */
    int i2block; /* pointer to doubly indirect block */
    int i3block; /* pointer to triply indirect block */
    int last_block_index; // the block index of the last data block used for this file
} inode;

typedef struct mount_table_entry {
    boolean free_spot;
    inode *mounted_inode; //directory at which this was mounted
    FILE *disk_image_ptr;
    // TODO. a disksize field
    superblock *superblock1;
} mount_table_entry;

typedef struct file_table_entry {
    //So change this to inode_index
    boolean free_file;
    inode *file_inode;
    int byte_offset; //byte offset into the file
    int access;
} file_table_entry;

/* Methods */
int f_open(char* filepath, int access, permission_value *permissions);
int f_read(void *buffer, int size, int n_times, int file_descriptor);
int f_write(void* buffer, int size, int ntimes, int fd );
boolean f_close(int file_descriptor);
boolean f_seek(int file_descriptor, int offset, int whence);
boolean f_rewind(int file_descriptor);
boolean f_stat(char *filepath, stat *st);
boolean f_remove(char *filepath);
directory_entry* f_opendir(char* filepath);
directory_entry* f_readdir(int index_into_file_table);
boolean f_closedir(directory_entry *entry);
boolean f_mount(char *disk_img, char *mounting_point, int *mount_table_index);
/* TODO - TBD: f_mkdir, f_rmdir, and f_unmount*/

/* Helper Methods */
boolean setup();
boolean shutdown();
void print_inode (inode* entry);
void print_table_entry (file_table_entry *entry);
void print_superblock(superblock *superblock1);
void print_file_table();
void get_filepath_and_filename(char *filepath, char **filename_to_return, char **path_to_directory); //TODO: ask Rose about expected behavior...
inode *get_inode_from_file_table_from_directory_entry(directory_entry *entry, int *table_index);
int update_single_inode_ondisk(inode* new_inode, int new_inode_index);

void direct_copy(directory_entry *entry, inode *current_directory, long block_to_fetch, long offset_in_block);
void indirect_copy(directory_entry *entry, inode *current_directory, int index, long indirect_block_to_fetch, long offset_in_block);

//getting a new free block, and updating the superblock, wrting to the disk.
//return value: index of the free block
int request_new_block();
int update_superblock_ondisk(superblock* new_superblock);
void *get_block_from_index(int block_index, inode *file_inode, int *data_region_index);
void *get_data_block(int index);
void free_data_block(void *block_to_free);
//intended to write to data region
int write_data_to_block(int block_index, void* content, int size);
int already_in_table(inode* node);
int find_next_freehead();
void set_permissions(permission_value *old_value, permission_value *new_value);

inode* get_inode(int index);
//filepath must be absolute path
// validity* checkvalidity(char *filepath);

//methods for testing
int first_free_location_in_mount();
int second_free_location_in_table();
int first_free_inode();
file_table_entry *get_table_entry(int index);
mount_table_entry *get_mount_table_entry(int index);
int get_fd_from_inode_value(int inode_index);
directory_entry get_last_directory_entry(int fd);

#endif //HW7_FILESYSTEM_H
