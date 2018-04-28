//
// Created by Sarah Depew on 4/25/18.
//

#include "filesystem.h"
#include "boolean.h"
#include "stdlib.h"

//global variables and arrays
mount_table_entry* mounted_disks[MOUNTTABLESIZE];
file_table_entry* file_table[FILETABLESIZE];
FILE *current_disk;
inode *root_inode;

//TODO: remove once a libaray
int main() {
    exit(0);
}

//the shell must call this method to set up the global variables and structures
boolean setup() {
    for (int i = 0; i < MOUNTTABLESIZE; i++) {
        mounted_disks[i] = malloc(sizeof(mount_table_entry));
        mounted_disks[i]->free_spot = TRUE;
    }

    for (int j = 0; j < FILETABLESIZE; j++) {
        file_table[j] = malloc(sizeof(file_table_entry));
        file_table[j]->free_file = TRUE;
    }

    root_inode = malloc(sizeof(inode));

    return TRUE;
}

//do this upon shell exit to ensure no memory leaks
boolean shutdown() {
    for (int i = 0; i < MOUNTTABLESIZE; i++) {
        free(mounted_disks[i]);
    }

    for (int j = 0; j < FILETABLESIZE; j++) {
        free(file_table[j]);
    }

    free(root_inode);

    return TRUE;
}

boolean f_mount(char *disk_img, char *mounting_point) {
    //open the disk
    //TODO: check that the disk image actually exists...
    //TODO: actually do something with mounting_point value passed in...location to mount (NOT ALWAYS ROOT!)
    int free_index = -1;

    //look for empty index into fmount table
    for (int i = 0; i < MOUNTTABLESIZE; i++) {
        if (mounted_disks[i]->free_spot == TRUE) {
            free_index = i;
            break;
        }
    }

    if (free_index != -1) {
        FILE *file_to_mount = fopen(disk_img, "rb+");
        current_disk = file_to_mount; 
        if (current_disk == NULL){
          printf("%s\n", "open Disk failed.");
          return EXIT_FAILURE;
        }
        int disksize = ftell(current_disk);
        if (disksize <0){
          printf("%s\n", "Disk invalid size. ");
        }
        //skip the boot block
        mounted_disks[free_index]->free_spot = FALSE;
        mounted_disks[free_index]->disk_image_ptr = file_to_mount;
        rewind(file_to_mount);
        fseek(file_to_mount, SIZEOFBOOTBLOCK, SEEK_SET); //place the file pointer at the superblock
        fread(mounted_disks[free_index]->superblock1, SIZEOFSUPERBLOCK, 1, file_to_mount);
        print_superblock(mounted_disks[free_index]->superblock1);
        //TODO: figure out what to do with inodes and pointing to them (remaining values in the structs)

        return TRUE;
    } else {
        return FALSE; //a spot was not found
    }

    return FALSE;
}

/* f_open() method */ //TODO: assume this is the absolute file path
int f_open(char* filepath, int access, permission_value* permissions) {
    //TODO: trace filepath and get index of inode
    //TODO: need to decide whether to check permission everytime, if yes: should do this while tracing
    //TODO: if this is a new file, then set permissions

    int index_of_inode = 0;
    //obtain the proper inode on the file //TODO: ask dianna how to do this? (page 326)
    file_table[0] = malloc(sizeof(file_table_entry));
    file_table[0]->file_inode = malloc(sizeof(inode));

    rewind(current_disk);
    fseek(current_disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + 0 + index_of_inode * sizeof(inode),
          SEEK_SET); //boot + super + offset + number of inodes before
    fread(file_table[0]->file_inode, sizeof(inode), 1, current_disk);
    file_table[0]->access = access;
    file_table[0]->free_file = FALSE;
    file_table[0]->byte_offset = 0;

    //TODO: remove once done debugging
    printf("file table entry %p\n", file_table[0]);
    print_table_entry(file_table[0]);
    printf("file information------data block\n");
    rewind(current_disk);
    fseek(current_disk, sizeof(inode), SEEK_SET);
    char buffer[21];
    fread(buffer, sizeof(char), 20, current_disk);
    buffer[20] = 0;
    printf("%s\n", buffer);

    return 0; //TODO: fix with actual return value
}

void print_inode (inode *entry) {
  printf("disk identifier: %d\n", entry->disk_identifier);
  printf("parent inode index: %d\n", entry->parent_inode_index);
  printf("next inode: %d\n", entry->next_inode);
  printf("file size: %d\n", entry->size);
  printf("file uid: %d\n", entry->uid);
  printf("file gid: %d\n", entry->gid);
  printf("file ctime: %d\n", entry->ctime);
  printf("file mtime: %d\n", entry->mtime);
  printf("file atime: %d\n", entry->atime);
  printf("file type: %d\n", entry->type);
  printf("file permissions: %d\n", entry->permission);
  printf("inode index: %d\n", entry->inode_index);  //TODO: add printing out all file blocks as needed
  for(int i=0; i<N_DBLOCKS; i++) {
    printf("%d th block with inode index %d\n", i, entry->dblocks[i]);
  }
for(int j=0; j<N_IBLOCKS; j++) {
    printf("%d th block with inode index %d\n", j, entry->iblocks[j]);
  }
  printf("i2block index: %d\n", entry->i2block);
  printf("i3block index: %d\n", entry->i3block);
  printf("last block index: %d\n", entry->last_block_index);
}

void print_table_entry (file_table_entry *entry) {
  printf("free file: %d\n", entry->free_file);
  print_inode(entry->file_inode);
  printf("byte offset: %d\n", entry->byte_offset);
  printf("access information %d\n", entry->access);
}

void print_superblock(superblock *superblock1) {
    printf("size: %d\n", superblock1->size);
    printf("inode offset: %d\n", superblock1->inode_offset);
    printf("data offset: %d\n", superblock1->data_offset);
    printf("free inode: %d\n", superblock1->free_inode);
    printf("free block: %d\n", superblock1->free_block);
    printf("root dir: %d\n", superblock1->root_dir);
}
