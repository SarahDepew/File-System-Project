//
// Created by Sarah Depew on 4/25/18.
//

#include "filesystem.h"
#include "boolean.h"
#include "stdlib.h"

//global variables and arrays
FILE *mounted_disks[MOUNTTABLESIZE];
file_table_entry* file_table[FILETABLESIZE];
FILE *current_disk;
inode *root_inode;


boolean f_mount(char *disk_img, char *mounting_point) {
  //read the inode into memory
  current_disk = fopen(disk_img, "r+");
  if (current_disk == NULL){
    printf("%s\n", "open Disk failed.");
    return EXIT_FAILURE;
  }
  int disksize = ftell(current_disk);
  if (disksize <0){
    printf("%s\n", "Disk invalid size. ");
  }
  //skip the boot block

//read in superblock here
  //look for empty index into fmount table
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

    // rewind(current_disk);
    fseek(current_disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + 0 + index_of_inode*sizeof(inode),SEEK_SET); //boot + super + offset + number of inodes before
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
  printf("access information \n", entry->access);
}
