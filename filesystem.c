#include "filesystem.h"
#include "boolean.h"
#include "stdlib.h"

//global variables and arrays
mount_table_entry* mounted_disks[MOUNTTABLESIZE];
file_table_entry* file_table[FILETABLESIZE];
mount_table_entry *current_mounted_disk;
inode *root_inode;

//the shell must call this method to set up the global variables and structures
boolean setup() {
    for (int i = 0; i < MOUNTTABLESIZE; i++) {
        mounted_disks[i] = (mount_table_entry*)malloc(sizeof(mount_table_entry));
        mounted_disks[i]->free_spot = TRUE;
        mounted_disks[i]->superblock1 = malloc(sizeof(superblock));
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
        free(mounted_disks[i]->superblock1);
        free(mounted_disks[i]);
    }

    for (int j = 0; j < FILETABLESIZE; j++) {
        free(file_table[j]);
    }

    free(root_inode);

    return TRUE;
}

boolean f_mount(char *disk_img, char *mounting_point, int *mount_table_index) {
    //open the disk
    setup();
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
        mount_table_index = &free_index;
        FILE *file_to_mount = fopen(disk_img, "rb+");
        FILE *current_disk = file_to_mount;
        if (current_disk == NULL){
          printf("%s\n", "Open Disk failed.");
          return FALSE;
        }
        int disksize = ftell(current_disk);
        if (disksize <0){
          printf("%s\n", "Disk invalid size. ");
          return FALSE;
        }
        //skip the boot block
        mounted_disks[free_index]->free_spot = FALSE;
        mounted_disks[free_index]->disk_image_ptr = file_to_mount;
        fseek(file_to_mount, SIZEOFBOOTBLOCK, SEEK_SET); //place the file pointer at the superblock
        fread(mounted_disks[free_index]->superblock1, sizeof(superblock), 1, file_to_mount);
        current_mounted_disk = mounted_disks[free_index];
        print_superblock(mounted_disks[free_index]->superblock1);
        //for testing: find data block
        superblock* sp = mounted_disks[free_index]->superblock1;
        fseek(file_to_mount,(sp->data_offset)*sp->size+SIZEOFBOOTBLOCK+SIZEOFSUPERBLOCK, SEEK_SET);
        void* data_content = malloc(sizeof(sp->size));
        fread(data_content, sp->size, 1, file_to_mount);
        printf("%s\n", data_content+sizeof(int));
        free(data_content);
        //TODO: figure out what to do with inodes and pointing to them (remaining values in the structs)

        return TRUE;
    } else {
        return FALSE; //a spot was not found
    }

    return FALSE;
}

/* f_open() method */ //TODO: assume this is the absolute file path
int f_open(char* filepath, int access, permission_value *permissions) {
    FILE *current_disk = current_mounted_disk->disk_image_ptr;
    //TODO: trace filepath and get index of inode
    //TODO: need to decide whether to check permission everytime, if yes: should do this while tracing
    //TODO: if this is a new file, then set permissions

    int index_of_inode = 0;
    //obtain the proper inode on the file //TODO: ask dianna how to do this? (page 326)
    file_table[0] = malloc(sizeof(file_table_entry));
    file_table[0]->file_inode = malloc(sizeof(inode));

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
    fseek(current_disk, sizeof(inode), SEEK_SET);
    char buffer[21];
    fread(buffer, sizeof(char), 20, current_disk);
    buffer[20] = 0;
    printf("%s\n", buffer);

    return 0; //TODO: fix with actual return value
}

int f_write(void* buffer, int size, int ntimes, int fd ){
    //check if the file accociated with this fd has been open
    if (file_table[fd] == NULL){
      printf("%s\n", "The file must be open before write");
      exit(EXIT_FAILURE);
    }
    if (file_table[fd]->file_inode->type == REG){
      printf("%s\n", "writing to a regular file");
      int old_size = file_table[fd]->byte_offset;
      superblock* sp = current_mounted_disk->superblock1;
      if (current_mounted_disk->free_spot == FALSE){
        printf("%s\n", "Not having enough sapce on the disk.");
        return (EXIT_FAILURE);
      }
      int new_size = old_size + sizeof(buffer);
      //check if the new_size is going to exceed the disk size. TODO
      //now assume the new_size is smaller than the disk size
      void* datatowrite = malloc(sizeof(buffer));
      fwrite(buffer, 1, sizeof(buffer), datatowrite);
      int start_byte = file_table[fd]->byte_offset;
      int lefttowrite = sizeof(buffer);
      int offset = 0;
      int free_byte = sp->size - start_byte;
      //calculate the right startplace_disk using file_size. TODO
      //writing to the first dblock right now. Need to change in the future. TODO.
      void* startplace_disk = (void*)(sp) +((file_table[fd]->file_inode->dblocks[0])*sp->size + (sp->data_offset*sp->size));
      while (lefttowrite > 0){
        //trace the disk to find the data block
        fwrite(datatowrite + offset, 1, free_byte, startplace_disk);
        offset += free_byte;
        free_byte = sp->size;
        lefttowrite -= sp->size;
      }
      //updating the offset in the file_table_entry
      file_table[fd]->byte_offset = new_size;
      free(datatowrite);
      return sizeof(buffer);
    } else if (file_table[fd]->file_inode->type == DIR){
      printf("%s\n", "writing to a dir file");
      //make_dir should do the same thing
    }
    return EXIT_SUCCESS;
}

boolean f_close(int file_descriptor) {
  if(file_descriptor>=FILETABLESIZE) {
    return FALSE;
  } else {
    file_table[file_descriptor]->free_file = TRUE;
    free(file_table[file_descriptor]->file_inode);
    return TRUE;
  }
  return FALSE;
}

boolean f_rewind(int file_descriptor) {
  if(file_descriptor>=FILETABLESIZE) {
    return FALSE;
  } else {
    file_table[file_descriptor]->byte_offset = 0;
    return TRUE;
  }
}

boolean f_stat(char *filepath, stat *st) {
  //TODO: use filepath to get to the intended inode
  int file_table_index = 0;
  //TODO: go to disk, trace the blocks, and return the inode of the file from the disk

  inode *inode1 = file_table[file_table_index]->file_inode;
  st->size = inode1->size;
  st->uid = inode1->uid;
  st->gid = inode1->gid;
  st->ctime = inode1->ctime;
  st->mtime = inode1->mtime;
  st->atime = inode1->atime;
  st->type = inode1->type;
  st->permission = inode1->permission;
  st->inode_index = inode1->inode_index;

  return TRUE;
}

//filepath must be absolute path
// validity* checkvalidity(char* filepath){
//   //parse filepath with '/'
// }

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
