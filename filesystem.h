//
// Created by Sarah Depew on 4/25/18.
//

#ifndef HW7_FILESYSTEM_H
#define HW7_FILESYSTEM_H

#include "boolean.h"
#include "stdio.h"


#define N_DBLOCKS 10
#define N_IBLOCKS 4
//change this to OPENFILE_MAX?
#define FILETABLESIZE 20
// #define FILENAME_MAX 40
#define MOUNTTABLESIZE 20
#define SIZEOFSUPERBLOCK 512
#define SIZEOFBOOTBLOCK 512

enum filetype {DIR, REG};
enum fileseek {SSET, SCUR, SEND};
enum permission{SUPER, REGULAR};
enum access{READ, WRITE, READANDWRITE, APPEND};

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
    char filename[FILENAME_MAX]; //simply the file name and not the absolute path
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
    int gid; /* owner’s group ID */ //TODO: do we have to have this?
    int ctime; /* last status change time */
    int mtime; /* last data modification time */
    int atime; /* last access time */
    int type; // dir or regular file
    int permission;
    int inode_index; // the index number for this inode
} stat;

/* inode struct */
typedef struct inode {
    unsigned int disk_identifier; //identifies the disk (the one that is mounted (for eventual removal)) //TODO: ask dianna about this
    //struct inode *parent;
    int parent_inode_index;
    int next_inode; /* index of next free inode */
    int size; /* number of bytes in file */
    int uid; /* owner’s user ID */
    int gid; /* owner’s group ID */
    int ctime; /* last status change time */
    int mtime; /* last data modification time */
    int atime; /* last access time */
    int type; // dir or regular file
    int permission; //TODO: change this to permission type
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

typedef struct cluster {
    boolean read;
    boolean write;
    boolean execute;
} cluster;

//TODO: ask dianna about how to model this value? (byte operators)
typedef struct permission_value {
    struct cluster *cluster_owner;
    struct cluster *cluster_group;
    struct cluster *cluster_others;
} permission_value;

/* Methods */
boolean f_mount(char *disk_img, char *mounting_point, int *mount_table_index);
int f_open(char* filepath, int access, permission_value *permissions);
int f_write(void* buffer, int size, int ntimes, int fd );
boolean f_close(int file_descriptor);
boolean f_rewind(int file_descriptor);
boolean f_stat(char *filepath, stat *st);

/* Helper Methods */
boolean setup();
boolean shutdown();
void print_inode (inode* entry);
void print_table_entry (file_table_entry *entry);
void print_superblock(superblock *superblock1);

#endif //HW7_FILESYSTEM_H
