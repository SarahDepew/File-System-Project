#include "filesystem.h"
#include "boolean.h"
#include "stdlib.h"

//global variables and arrays
mount_table_entry* mounted_disks[MOUNTTABLESIZE];
file_table_entry* file_table[FILETABLESIZE];
mount_table_entry *current_mounted_disk;
inode *root_inode;
int table_freehead = 0;

//the shell must call this method to set up the global variables and structures
boolean setup() {
    printf("Size of directory struct: %lu\n", sizeof(directory_entry));
    for (int i = 0; i < MOUNTTABLESIZE; i++) {
        mounted_disks[i] = (mount_table_entry *) malloc(sizeof(mount_table_entry));
        mounted_disks[i]->free_spot = TRUE;
        mounted_disks[i]->superblock1 = malloc(sizeof(superblock));
    }

    for (int j = 0; j < FILETABLESIZE; j++) {
        file_table[j] = malloc(sizeof(file_table_entry));
        file_table[j]->free_file = TRUE;
    }

    current_mounted_disk = malloc(sizeof(mount_table_entry));

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

    free(current_mounted_disk);

    return TRUE;
}

boolean f_mount(char *disk_img, char *mounting_point, int *mount_table_index) {
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
        mount_table_index = &free_index;
        FILE *file_to_mount = fopen(disk_img, "rb+");
        FILE *current_disk = file_to_mount;
        if (current_disk == NULL) {
            printf("%s\n", "Open Disk failed.");
            return FALSE;
        }
        int disksize = ftell(current_disk);
        if (disksize < 0) {
            printf("%s\n", "Disk invalid size. ");
            return FALSE;
        }
        //skip the boot block
        mounted_disks[free_index]->free_spot = FALSE;
        mounted_disks[free_index]->disk_image_ptr = file_to_mount;
        fseek(file_to_mount, SIZEOFBOOTBLOCK, SEEK_SET); //place the file pointer at the superblock
        fread(mounted_disks[free_index]->superblock1, sizeof(superblock), 1, file_to_mount);
        current_mounted_disk = mounted_disks[free_index];

        print_superblock(mounted_disks[free_index]->superblock1); //TODO: remove at some point...

        //for testing: find data block
        superblock *sp = mounted_disks[free_index]->superblock1;
        fseek(file_to_mount, (sp->data_offset) * sp->size + SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK, SEEK_SET);
        void *data_content = malloc(sizeof(char) * sp->size);
        fread(data_content, sp->size, 1, file_to_mount);

        printf("%s\n", (char *) data_content + sizeof(int));

        free(data_content);
        //TODO: figure out what to do with inodes and pointing to them (remaining values in the structs)

        return TRUE;
    } else {
        return FALSE; //a spot was not found
    }

    return FALSE;
}

/*

void get_filepath_and_filename(char *filepath, char **filename_to_return, char **path_to_directory){
  //get the filename and the path seperately
  char *filename = NULL;
  char *path = malloc(strlen(filepath));
  char path_copy[strlen(filepath) + 1];
  char copy[strlen(filepath) + 1];
  strcpy(path_copy, filepath);
  strcpy(copy, filepath);
  char *s = "/'";
  //calculate the level of depth of dir
  char *token = strtok(copy, s);
  int count = 0;
  while (token != NULL) {
      count++;
      token = strtok(NULL, s);
  }
  // printf("count : %d\n", count);
  filename = strtok(path_copy, s);
  while (count > 1) {
      count--;
      path = strcat(path, "/");
      path = strcat(path, filename);
      filename = strtok(NULL, s);
  }

  *filename_to_return = filename;
  *path_to_directory = path;
}
*/

/* f_open() method */ //TODO: assume this is the absolute file path, TODO: add permissions functionality
int f_open(char* filepath, int access, permission_value *permissions) {
    //FILE *current_disk = current_mounted_disk->disk_image_ptr;
    //TODO: need to decide whether to check permission everytime, if yes: should do this while tracing
    //TODO: if this is a new file, then set permissions

    //get the filename and the path seperately
    char *filename = NULL;
    char *path = malloc(strlen(filepath));
    char path_copy[strlen(filepath) + 1];
    char copy[strlen(filepath) + 1];
    strcpy(path_copy, filepath);
    strcpy(copy, filepath);
    char *s = "/'";
    //calculate the level of depth of dir
    char *token = strtok(copy, s);
    int count = 0;
    while (token != NULL) {
        count++;
        token = strtok(NULL, s);
    }
    // printf("count : %d\n", count);
    filename = strtok(path_copy, s);
    while (count > 1) {
        count--;
        path = strcat(path, "/");
        path = strcat(path, filename);
        filename = strtok(NULL, s);
    }
    printf("path: %s\n", path);
    printf("filename: %s\n", filename);
    //TODO: need to check if the file is already in the file_table. Any sart way of doing that?
    directory_entry *dir = f_opendir(path);
    if (dir == NULL) {
        printf("%s\n", "directory does not exist");
        free(path);
        return EXIT_FAILURE;
    } else {
        //directory exits, need to check if the file exits
        printf("%s\n", "directory exits. GOOD news.");
        int dir_node_index = dir->inode_index;
        printf("dir_index: %d\n", dir_node_index);
        printf("dir_name: %s\n", dir->filename);
        inode *dir_node = get_inode(dir_node_index);
        //go into the dir_entry to find the inode of the file
        int block_offset = 0; //the offset within a data block
        int block_index = 0; //block index in the data region
        int file_offset = 0; //the offset within the whole directory file
        //check dblocks
        printf("size: %d\n", dir_node->size);
        for (int i = 0; i < N_DBLOCKS; i++) {
            if (file_offset >= dir_node->size) {
                printf("%s\n", "Reach the end of the directory");
                break;
            }
            block_index = dir_node->dblocks[i];
            void *block = get_data_block(block_index);
            for (; block_offset <= BLOCKSIZE && file_offset < dir_node->size; block_offset += sizeof(directory_entry)) {
                directory_entry *entry = (directory_entry *) (block + block_offset);
                char *name_found = entry->filename;
                if (strcmp(filename, name_found) == 0) {
                    printf("%s found\n", name_found);
                    printf("%d\n", table_freehead);
                    file_table_entry *file_entry = malloc(sizeof(file_table_entry));
                    file_entry->free_file = FALSE;
                    file_entry->file_inode = get_inode(entry->inode_index);
                    file_entry->byte_offset = 0;
                    file_table[table_freehead] = file_entry;
                    free(path);
                    print_file_table();
                    int fd = table_freehead;
                    table_freehead = find_next_freehead();
                    return fd;
                }
                file_offset += sizeof(directory_entry);
            }
        }
        //TODO. go into idirect blocks
        for (int i = 0; i < N_IBLOCKS; i++) {
            if (file_offset >= dir_node->size) {
                printf("%s\n", "Reach the end of the directory");
                break;
            }
        }
    }
    // print_file_table();
    return EXIT_SUCCESS;
}

int f_write(void* buffer, int size, int ntimes, int fd ) {
    //check if the file accociated with this fd has been open
    if (file_table[fd]->free_file == TRUE) {
        printf("%s\n", "The file must be open before write");
        exit(EXIT_FAILURE);
    }
    if (file_table[fd]->file_inode->type == REG) {
        printf("%s\n", "writing to a regular file");
        int old_size = file_table[fd]->byte_offset;
        superblock *sp = current_mounted_disk->superblock1;
        if (current_mounted_disk->free_spot == FALSE) {
            printf("%s\n", "Not having enough sapce on the disk.");
            return (EXIT_SUCCESS);
        }
        int new_size = old_size + sizeof(buffer);
        //check if the new_size is going to exceed the disk size. TODO
        //now assume the new_size is smaller than the disk size
        void *datatowrite = malloc(sizeof(buffer));
        fwrite(buffer, 1, sizeof(buffer), datatowrite);
        int start_byte = file_table[fd]->byte_offset;
        int lefttowrite = sizeof(buffer);
        int offset = 0;
        int free_byte = sp->size - start_byte;
        //calculate the right startplace_disk using file_size. TODO
        //writing to the first dblock right now. Need to change in the future. TODO.
        //WARNING: consider the first int that links every block together
        void *startplace_disk = (void *) (sp) + sizeof(int) +
                                ((file_table[fd]->file_inode->dblocks[0]) * sp->size + (sp->data_offset * sp->size));
        while (lefttowrite > 0) {
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
    } else if (file_table[fd]->file_inode->type == DIR) {
        printf("%s\n", "writing to a dir file");
        //make_dir should do the same thing
    }
    return EXIT_SUCCESS;
}

//TODO: update table_freehead here?
boolean f_close(int file_descriptor) {
    if (file_descriptor >= FILETABLESIZE) {
        return FALSE;
    } else {
        file_table[file_descriptor]->free_file = TRUE;
        free(file_table[file_descriptor]->file_inode);
        //TODO: figure out how to free permissions...
        return TRUE;
    }
    return FALSE;
}

boolean f_rewind(int file_descriptor) {
    if (file_descriptor >= FILETABLESIZE) {
        return FALSE;
    } else {
        file_table[file_descriptor]->byte_offset = 0;
        return TRUE;
    }
}

boolean f_stat(char *filepath, stat *st) {
    permission_value *permissions = malloc(sizeof(permission_value));
    permissions->cluster_owner = malloc(sizeof(cluster));
    permissions->cluster_owner->read = TRUE;
    permissions->cluster_owner->write = TRUE;
    permissions->cluster_owner->execute = TRUE;
    permissions->cluster_group = malloc(sizeof(cluster));
    permissions->cluster_group->read = TRUE;
    permissions->cluster_group->write = TRUE;
    permissions->cluster_group->execute = TRUE;
    permissions->cluster_others = malloc(sizeof(cluster));
    permissions->cluster_others->read = TRUE;
    permissions->cluster_others->write = TRUE;
    permissions->cluster_others->execute = TRUE;
    int fd = f_open(filepath, READ, permissions);

    inode *inode1 = file_table[fd]->file_inode;
    st->size = inode1->size;
    st->uid = inode1->uid;
    st->gid = inode1->gid;
    st->ctime = inode1->ctime;
    st->mtime = inode1->mtime;
    st->atime = inode1->atime;
    st->type = inode1->type;
    st->permission = inode1->permission;
    st->inode_index = inode1->inode_index;

    f_close(fd);
    return TRUE;
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
    for (int i = 0; i < N_DBLOCKS; i++) {
        printf("%d th block with inode index %d\n", i, entry->dblocks[i]);
    }
    for (int j = 0; j < N_IBLOCKS; j++) {
        printf("%d th block with inode index %d\n", j, entry->iblocks[j]);
    }
    printf("i2block index: %d\n", entry->i2block);
    printf("i3block index: %d\n", entry->i3block);
    printf("last block index: %d\n", entry->last_block_index);
}

void print_file_table() {
    printf("%s\n", "------------start printing file_table------");
    for (int i = 0; i < FILETABLESIZE; i++) {
        if (file_table[i] != NULL) {
            if (file_table[i]->free_file == TRUE) {
                printf("%d: %s\n", i, "empty");
            } else {
                inode *node = file_table[i]->file_inode;
                printf("%d: inode->index: %d \t byte_offset: %d\n", i, node->inode_index, file_table[i]->byte_offset);
            }
        }
    }
    printf("%s\n", "------done printing-----------");
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

directory_entry* f_opendir(char* filepath){
  //parse the filepath
  char path[strlen(filepath)+1];
  char copy[strlen(filepath)+1];
  strcpy(path, filepath);
  strcpy(copy, filepath);
  char* s  = "/'";
  char* token;
  //calculate the level of depth
  token = strtok(copy, s);
  int count = 0;
  while(token != NULL){
    count ++;
    token = strtok(NULL, s);
  }
  // printf("count : %d\n", count);

  //create directory_entry for the root
  directory_entry* entry = malloc(sizeof(directory_entry));
  entry->inode_index = 0;
  strcpy(entry->filename, "/");
  directory_entry* dir_entry = NULL;
  inode* root_node = get_inode(0);

  //add root directory to the open_file_table. Assume it is found.
  //if root alreay exists, should not add any more.
  // if (file_table[0] != NULL){
  if (file_table[0]->free_file == TRUE){
    file_table_entry* root_table_entry = malloc(sizeof(file_table_entry));
    root_table_entry->free_file = FALSE;
    root_table_entry->file_inode = root_node;
    root_table_entry->byte_offset = 0;
    file_table[0] = root_table_entry;
  }
  table_freehead = find_next_freehead();
  // }
  token = strtok(path, s);
  // printf("%s\n", "testing, before while");
  int i = 0;
  while(token != NULL && count >= 1){
    // printf("%s\n", token);
    //get the directory entry
    int found = FALSE;
    file_table[i]->byte_offset = 0;
    while(found ==  FALSE){
      // printf("%s\n", "second while");
      dir_entry = f_readir(i);
      if (dir_entry == NULL){
        //reach the last byte in the file
        break;
      }
      char* name = dir_entry->filename;
      if(strcmp(name, token) == 0){
        printf("%s\n", "found");
        found = TRUE;
      }
    }
    if (i != 0){
      printf("%s\n", "here");
      //remove the ith index from the table. NEED CHECK. ROSE
      // file_table[i] = NULL;
      // table_freehead = i;
      free(file_table[i]);
    }else{
      //root is always in the table. won't be removed.
      table_freehead = find_next_freehead();
      i = table_freehead;
    }
    if(found == FALSE){
      printf("%s is not found\n",token);
      return NULL;
    }
    count -- ;
    entry = dir_entry;
    //add dir_entry to the table
    //need to check if the dir_entry already exists
    inode* node = get_inode(dir_entry->inode_index);
    printf("already_in_table: %d\n", already_in_table(node));
    if(already_in_table(node) == -1 ){
      file_table_entry* table_ent = malloc(sizeof(file_table_entry));
      table_ent->free_file = FALSE;
      table_ent->file_inode = node;
      table_ent->byte_offset = 0;
      file_table[i] = table_ent;
    }
    //update table_freehead
    token = strtok(NULL, s);
  }
  printf("%s\n", "end of open_dir-----");
  // print_file_table();
  return entry;
}

//TODO: update the time with the last accessed time, here!
int f_read(void *buffer, int size, int n_times, int file_descriptor) {
    inode *file_to_read = file_table[file_descriptor]->file_inode;
    long file_offset = file_table[file_descriptor]->byte_offset;
    int block_to_read = file_offset / BLOCKSIZE;
    // int block_to_read = 0;
    int bytes_to_read;
    int bytes_remaining_in_block;
    int block_offset;
    char *block_to_read_from;
    //TODO:check that size isn't larger than the file size and check that size*n_times isn't larger than the file size
    //TODO: check that you are not reading past the end of the disk...
    //TODO: check if you're allowed to read the file!!
    int buffer_index = 0;
    char *buffer_to_return = malloc(sizeof(size * n_times));

    //code to repeat
    for (int i = 0; i < n_times; i++) {
        bytes_to_read = size;
        block_offset = file_offset % 512;
        while (size > 0) {
            block_to_read_from = get_block_from_index(block_to_read, file_to_read);
            bytes_remaining_in_block = 512 - block_offset - 1;
            if (bytes_remaining_in_block >= size) {
                bytes_to_read = size;
                //TODO: TRANSFER BLOCK DATA
                for (int j = 0; j < bytes_to_read; j++) {
                    buffer_to_return[buffer_index] = block_to_read_from[block_offset];
                    buffer_index++;
                    block_offset++;
                }
                size -= bytes_to_read;
                free_data_block(block_to_read_from);
            } else { //bytes_remaining_in_block < size
                bytes_to_read = bytes_remaining_in_block;
                //TODO: TRANSFER BLOCK DATA
                for (int k = 0; k < bytes_to_read; k++) {
                    buffer_to_return[buffer_index] = block_to_read_from[block_offset];
                    buffer_index++;
                    block_offset++;
                }
                size -= bytes_to_read;
                block_to_read++;
                free_data_block(block_to_read_from);
            }
        }
    }
    printf("contents of buffer: %s\n", buffer_to_return);
    return TRUE;
}

//TODO: pont define and figure out if you're requesting a block that doesn't exist
//TODO: check that this isn't just off by one
void *get_block_from_index(int block_index, inode *file_inode) { //block index is block overall to get...more computations must be done to know which block precisely to obtain
  //get direct block
  block *block_to_return = NULL;
  if (block_index >= 0 && block_index <= 9) {
      block_to_return = get_data_block(block_index);
  } else if (block_index >= 10 && block_index <= 521) {
      block_index -= 10; //adjust to array here
      int indirect_array_index = block_index / 128;
      int array_index = block_index % 128;
      int *indirect_block = get_data_block(file_inode->iblocks[indirect_array_index]);
      block_to_return = get_data_block(indirect_block[array_index]);
      free_data_block(indirect_block);
  } else if (block_index >= 522 && block_index <= 16905) {
      block_index -= 522; //adjust to array here
      int indirect_array_index = block_index / 128;
      int array_index = block_index % 128;
      int *double_indirect_block = get_data_block(file_inode->i2block);
      int *indirect_block = get_data_block(double_indirect_block[indirect_array_index]);
      block_to_return = get_data_block(indirect_block[array_index]);
      free_data_block(double_indirect_block);
      free_data_block(indirect_block);
  } else if (block_index >= 16906 && block_index <= 2114057) {
      block_index -= 16906;
      int double_indirect_array_index = block_index / 128 / 128;
      int indirect_array_index = block_index / 128;
      int array_index = block_index % 128;
      int *triple_indirect_block = get_data_block(file_inode->i3block);
      int *double_indirect_block = get_data_block(triple_indirect_block[double_indirect_array_index]);
      int *indirect_block = get_data_block(double_indirect_block[indirect_array_index]);
      block_to_return = get_data_block(indirect_block[array_index]);
      free_data_block(triple_indirect_block);
      free_data_block(double_indirect_block);
      free_data_block(indirect_block);
  } else {
      printf("Index out of bounds for get block from index.\n");
  }
  return block_to_return;
}

boolean f_remove(char *filepath) {
  //TODO: make a method for the following (ask Rose)
  //get the filename and the path seperately
  char *filename = NULL;
  char *path = malloc(strlen(filepath));
  char path_copy[strlen(filepath) + 1];
  char copy[strlen(filepath) + 1];
  strcpy(path_copy, filepath);
  strcpy(copy, filepath);
  char *s = "/'";
  //calculate the level of depth of dir
  char *token = strtok(copy, s);
  int count = 0;
  while (token != NULL) {
      count++;
      token = strtok(NULL, s);
  }
  // printf("count : %d\n", count);
  filename = strtok(path_copy, s);
  while (count > 1) {
      count--;
      path = strcat(path, "/");
      path = strcat(path, filename);
      filename = strtok(NULL, s);
  }
  printf("path: %s\n", path);
  printf("filename: %s\n", filename);
  //TODO: need to check if the file is already in the file_table. Any sart way of doing that?
  directory_entry *dir = f_opendir(path);
  if (dir == NULL) {
      printf("%s\n", "directory does not exist");
      free(path);
      return FALSE;
  } else {
    int index = -1;
    inode *directory_inode = get_inode_from_file_table_from_directory_entry(dir, &index);
    if(directory_inode == NULL) {
      return FALSE;
    } else {
      //now, I have the inode for the file...

      //TODO: Remove the file from its directory
      directory_entry *current_entry = f_readir(index);
      directory_entry *directory_to_delete = NULL;
      while(current_entry != NULL && strcmp(current_entry->filename, filename)!=0) {
        current_entry = f_readir(index);
      }

      if(current_entry == NULL) {
        return FALSE; //wasn't able to find the entry in the directory
      } else {
        directory_to_delete = current_entry;
      }

      //need to write directory_to_delete back to the disk, now, since it has been deleted...
      directory_to_delete->inode_index = -1;
      memset(directory_to_delete->filename, 0, FILENAMEMAX);
      // fseek(); //TODO: replace with our fseek when needed
      file_table[index]->byte_offset -= sizeof(directory_entry); //pointer in file is now at the correct location to fwrite...
      fseek(current_mounted_disk->disk_image_ptr, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + current_mounted_disk->superblock1->data_offset * current_mounted_disk->superblock1->size + file_table[index]->byte_offset, SEEK_SET);
      fwrite(directory_to_delete, sizeof(directory_entry), 1, current_mounted_disk->disk_image_ptr);

      //TODO: Release the i-node to the pool of free inodes and re-write the value to the disk...
      superblock *superblock1 = current_mounted_disk->superblock1;
      //TODO: talk to Rose and see what else needs to be done here...
      int new_head = directory_inode->inode_index;
      int old_head = superblock1->free_inode;

      //read in the current_head inode and write the new and correct value to disk...
      superblock1->free_inode = new_head;
      directory_inode->next_inode = old_head;

      fseek(current_mounted_disk->disk_image_ptr, SIZEOFBOOTBLOCK, SEEK_SET);
      fwrite(superblock1, SIZEOFSUPERBLOCK, 1, current_mounted_disk->disk_image_ptr);

      fseek(current_mounted_disk->disk_image_ptr, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + current_mounted_disk->superblock1->inode_offset * current_mounted_disk->superblock1->size + new_head * sizeof(inode), SEEK_SET);
      fwrite(superblock1, SIZEOFSUPERBLOCK, 1, current_mounted_disk->disk_image_ptr);




      //TODO: return the disk blocks to the pool of free disk blocks and WRITE THE SUPERBLOCK TO DISK!!
    }
  }
}

inode *get_inode_from_file_table_from_directory_entry(directory_entry *entry, int *table_index) {
  if(entry == NULL) {
    return NULL;
  } else {
    int inode_index = directory_entry->inode_index;
    inode *directory_inode = NULL;
    for(int i=0; i<FILETABLESIZE; i++){
      if(!file_table[i]->free_file && file_table[i]->file_inode->inode_index == inode_index) {
        directory_inode = file_table[i]->file_inode;
        *table_index = i;
      }
    }
  }

  return directory_inode;
}

//TODO: add skipping empty directory entries...
directory_entry* f_readir(int index_into_file_table) {
    //TODO: error check here for valid index into file... (not trying to read past end of file)

    //fetch the inode of the directory to be read (assume the directory file is already open in the file table and that you are given the index into the file table)
    long offset_into_file = file_table[index_into_file_table]->byte_offset;
    inode *current_directory = file_table[index_into_file_table]->file_inode;
    superblock *superblockPtr = current_mounted_disk->superblock1;
    // long directory_index_in_block = offset_into_file/sizeof(directory_entry); //TODO: make directory padded so that we have block divisible by directory entries
    long block = ((float) offset_into_file /
                  (float) superblockPtr->size);
    directory_entry *next_directory = malloc(sizeof(directory_entry));
    long offset_in_block = offset_into_file - (superblockPtr->size * block);
    long num_indirect = superblockPtr->size / sizeof(int);
    // long num_directories = superblockPtr->size / sizeof(directory_entry);
    if (offset_into_file <= current_directory->size) {
        direct_copy(next_directory, current_directory, current_directory->dblocks[block], offset_in_block);
    } else if (offset_into_file > DBLOCKS && offset_into_file <= IBLOCKS) {
        long adjusted_block = block - N_DBLOCKS; //index into indirect block range
        long index_indirect = adjusted_block / num_indirect;
        long index = adjusted_block % num_indirect;
        indirect_copy(next_directory, current_directory, index, current_directory->iblocks[index_indirect],
                      offset_in_block);
    } else if (offset_into_file > IBLOCKS && offset_into_file <= I2BLOCKS) {
        long adjusted_block = block - N_DBLOCKS - N_IBLOCKS * num_indirect; //index into indirect block range
        long index_indirect = adjusted_block / num_indirect;
        long index = adjusted_block % num_indirect;

        int *indirect_block_array = get_data_block(current_directory->i2block);
        int block_to_fetch = *((int *) (indirect_block_array + index_indirect * sizeof(int)));
        free_data_block(indirect_block_array);

        indirect_copy(next_directory, current_directory, index, block_to_fetch, offset_in_block);
    } else if (offset_into_file > I2BLOCKS && offset_into_file <= I3BLOCKS) {
        long adjusted_block = block - N_DBLOCKS - N_IBLOCKS * num_indirect -
                              num_indirect * num_indirect; //index into double indirect block range
        long double_index_indirect = adjusted_block / num_indirect / num_indirect;
        int *doubly_indirect_block_array = get_data_block(current_directory->i3block);
        int indirect_block_to_fetch = *((int *) (doubly_indirect_block_array + double_index_indirect * sizeof(int)));
        free_data_block(doubly_indirect_block_array);

        long index_indirect = adjusted_block / num_indirect;
        long index = adjusted_block % num_indirect;
        int *indirect_block_array = get_data_block(indirect_block_to_fetch);
        int block_to_fetch = *((int *) (indirect_block_array + index_indirect * sizeof(int)));
        free_data_block(indirect_block_array);

        indirect_copy(next_directory, current_directory, index, block_to_fetch, offset_in_block);
    } else {
        free(next_directory);
        printf("File Size Too Large To Handle.\n");
        return NULL;
    }

    //increment offset into the file
    file_table[index_into_file_table]->byte_offset += sizeof(directory_entry);
    return next_directory;
}

void indirect_copy(directory_entry *entry, inode *current_directory, int index, long indirect_block_to_fetch, long offset_in_block) {
    void *indirect_data_block = get_data_block(indirect_block_to_fetch);
    int block_to_fetch = *((int *) (indirect_data_block + index * sizeof(int)));
    free_data_block(indirect_data_block);
    direct_copy(entry, current_directory, block_to_fetch, offset_in_block);
}

void direct_copy(directory_entry *entry, inode *current_directory, long block_to_fetch, long offset_in_block) {
    void *data_block = get_data_block(block_to_fetch);
    memcpy(entry, data_block + offset_in_block, sizeof(directory_entry));
    free_data_block(data_block);
}

//TODO: add error when you're trying to read a data block that doesn't exist on the disk (a.k.a past the end of the disk...)
void *get_data_block(int index) {
  void *data_block = malloc(current_mounted_disk->superblock1->size);
  FILE *current_disk = current_mounted_disk->disk_image_ptr;
  fseek(current_disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK +
    current_mounted_disk->superblock1->data_offset * current_mounted_disk->superblock1->size +
    index * current_mounted_disk->superblock1->size,
    SEEK_SET);
    fread(data_block, current_mounted_disk->superblock1->size, 1, current_disk);
    return data_block;
  }

int already_in_table(inode* node){
  int index2 = node->inode_index;
    for(int i=0; i<FILETABLESIZE; i++){
      if(file_table[i] != NULL){
        if (file_table[i]->free_file == FALSE){
          int index1 = file_table[i]->file_inode->inode_index;
          if (index1 == index2){
            return i;
          }
        }
      }
    }
    return -1;
}

int find_next_freehead(){
  for(int i =0;i<FILETABLESIZE; i++){
    if(file_table[i]->free_file == TRUE){
      return i;
    }
  }
  return EXIT_FAILURE;
}

void free_data_block(void *block_to_free) {
  free(block_to_free);
}

inode* get_inode(int index) {
  inode *node = malloc(sizeof(inode));
  FILE *current_disk = current_mounted_disk->disk_image_ptr;
  fseek(current_disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + index * sizeof(inode), SEEK_SET);
  fread(node, sizeof(inode), 1, current_disk);
  return node;
}
