//
// Created by Sarah Depew on 4/25/18.
//

#ifndef HW7_FILESYSTEM_H
#define HW7_FILESYSTEM_H

#include "boolean.h"

#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define FILETABLESIZE 20

enum filetype {DIR, REG};
enum fileseek {SSET, SCUR, SEND};
enum permission{SUPER, REGULAR};
//TODO: say why I did this
enum access{READ, WRITE, READANDWRITE, APPEND};

/* Directory entry struct modeled after V7 */
typedef struct directory_entry {
    int inode_ptr; //pointer to inode
    char filename[255]; //simply the file name and not the absolute path
} directory_entry;

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
    char filename[255]; //not absolute path, just name
    unsigned disk_identifier; //identifies the disk (the one that is mounted (for eventual removal)) //TODO: ask dianna about this
    //struct inode *parent;
    int parent_inode_index;
    int next_inode; /* index of next free inode */
//    struct stat *f_stats; //file statistics
//TODO: talk to Rose; this may be easier for the inode region
    int size; /* number of bytes in file */
    int uid; /* owner’s user ID */
    int gid; /* owner’s group ID */ //TODO: do we have to have this? (super and basic in different groups)
    int ctime; /* last status change time */
    int mtime; /* last data modification time */
    int atime; /* last access time */
    int type; // dir or regular file
    int permission;
    int inode_index; // the index number for this inode
    int dblocks[N_DBLOCKS]; /* pointers to data blocks */
    int iblocks[N_IBLOCKS]; /* pointers to indirect blocks */
    int i2block; /* pointer to doubly indirect block */
    int i3block; /* pointer to triply indirect block */
    int last_block_index; // the block index of the last data block used for this file //TODO: check if we actually need this??
} inode;

typedef struct mount_table_entry {
    inode *dir_mounted_inode; //what was just mounted
    inode *mounted_inode; //directory at which this was mounted
} mount_table_entry;

typedef struct file_table_entry {
    int file_inode; //TODO: ask dianna if this should be an inode ptr? (yes, because of fopen)
    int byte_offset; //byte offset into the file
} file_table_entry;

/*  To access file information corresponding to certain file use the file descriptor as the index of the 'entries' array */
typedef struct file_table {
    // number of open files
    int filenum;
    int free_fd_num; //number of unused file descriptor
    int* free_fd; // array storing the unused file descriptor
    // array of file table entries, number of entries equal to (filenum - 1)
    file_table_entry* entries;
} file_table;

typedef struct cluster {
    boolean read;
    boolean write;
    boolean execute;
} cluster;

//TODO: ask dianna about how to model this value? (byte operators)
typedef struct permission_value {
    struct cluster cluster_owner;
    struct cluster cluster_group;
    struct cluster cluster_others;
} permission_value;

/* Methods */
int f_open(char* filepath, int access, permission_value permissions);

#endif //HW7_FILESYSTEM_H
