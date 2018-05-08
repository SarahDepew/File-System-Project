#include <math.h>
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
  for (int i = 0; i < MOUNTTABLESIZE; i++) {
    mounted_disks[i] = (mount_table_entry *) malloc(sizeof(mount_table_entry));
    mounted_disks[i]->free_spot = TRUE;
    mounted_disks[i]->superblock1 = malloc(sizeof(superblock));
  }

  for (int j = 0; j < FILETABLESIZE; j++) {
    file_table[j] = (file_table_entry*)malloc(sizeof(file_table_entry));
    file_table[j]->file_inode = malloc(sizeof(inode));
    file_table[j]->free_file = TRUE;
  }

  return TRUE;
}

//do this upon shell exit to ensure no memory leaks
boolean shutdown() {
  for (int i = 0; i < MOUNTTABLESIZE; i++) {
    free(mounted_disks[i]->superblock1);
    free(mounted_disks[i]);
  }

  for (int j = 0; j < FILETABLESIZE; j++) {
    free(file_table[j]->file_inode);
    free(file_table[j]);
  }

  return TRUE;
}

int first_free_location_in_mount() {
  int index = -1;
  for(int i=0; i<MOUNTTABLESIZE; i++) {
    if(mounted_disks[i]->free_spot == TRUE) {
      index = i;
      break;
    }
  }

  return index;
}

int desired_free_location_in_table(int location_sought) {
  int num_locations = 1;
  int index = -1;
  for(int i=0; i<FILETABLESIZE; i++) {
    if(file_table[i]->free_file == TRUE) {
      if(num_locations < location_sought) {
        num_locations++;
      } else {
        index = i;
        break;
      }
    }
  }

  return index;
}

file_table_entry *get_table_entry(int index) {
  return file_table[index];
}

mount_table_entry *get_mount_table_entry(int index) {
  return mounted_disks[index];
}

int first_free_inode() {
  return current_mounted_disk->superblock1->free_inode;
}

int get_fd_from_inode_value(int inode_index) {
  int val = -1;
  for(int i=0; i<FILETABLESIZE; i++) {
    if(file_table[i]->free_file == FALSE && file_table[i]->file_inode->inode_index == inode_index) {
      val = i;
    }
  }

  return val;
}

directory_entry get_last_directory_entry(int fd){
  int directory_size = file_table[fd]->file_inode->size;
  int last_block_index = directory_size/BLOCKSIZE;
  int block_location = last_block_index%BLOCKSIZE;
  int directory_location = block_location/sizeof(directory_entry) - 1;
  inode *file_inode = file_table[fd]->file_inode;
  int data_region_index = -1;
  void *last_block = get_block_from_index(last_block_index, file_inode, &data_region_index);
  directory_entry *array = last_block;
  directory_entry value = array[directory_location];
  free_data_block(last_block);
  return value;
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
    *mount_table_index = free_index;
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

    //for testing: find data block
    superblock *sp = mounted_disks[free_index]->superblock1;
    fseek(file_to_mount, (sp->data_offset) * sp->size + SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK, SEEK_SET);
    void *data_content = malloc(sizeof(char) * sp->size);
    fread(data_content, sp->size, 1, file_to_mount);
    free(data_content);

    //TODO: figure out what to do with inodes and pointing to them (remaining values in the structs)
    return TRUE;
  } else {
    return FALSE; //a spot was not found
  }
  return FALSE;
}

boolean f_unmount(int mid) {
  if(mid >=0 && mid <MOUNTTABLESIZE){
    if(mounted_disks[mid]->free_spot == FALSE) {
      mounted_disks[mid]->free_spot = TRUE;
      fclose(mounted_disks[mid]->disk_image_ptr);
      // free(mounted_disks[mid]->mounted_inode); //not doing anything with this, yet...
      // free(mounted_disks[mid]->superblock1); //free this in the shutdown() method...
      return TRUE;
    } else {
      return FALSE;
    }
  } else {
    return FALSE;
  }
}

/* f_open() method */ //TODO: add permissions functionality //TODO: add access fulctonality with timing...
int f_open(char* filepath, int access, permission_value *permissions) {
  //get the filename and the path seperately
  char *filename = NULL;
  char *path = malloc(strlen(filepath));
  memset(path, 0, strlen(filepath));
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
    directory_entry* entry = NULL;
    //go into the dir_entry to find the inode of the file
    printf("size: %d\n", dir_node->size);
    int parent_fd = already_in_table(dir_node);
    file_table[parent_fd]->byte_offset = 0;
    for(int i=0; i<dir_node->size; i+= sizeof(directory_entry)){
      entry = f_readdir(parent_fd);
      printf("entry: %s\n", entry->filename );
      if (strcmp(entry->filename, filename) == 0){
        printf("%s found\n", entry->filename);
        file_table_entry *file_entry = file_table[table_freehead];
        file_entry->free_file = FALSE;
        free(file_entry->file_inode);
        inode *file_inode = get_inode(entry->inode_index);
        set_permissions(file_inode->permission, permissions);
        update_single_inode_ondisk(file_inode, file_inode->inode_index);
        file_entry->file_inode = file_inode;
        file_entry->byte_offset = 0;
        file_entry->access = access;
        free(path);
        print_file_table();
        int fd = table_freehead;
        table_freehead = find_next_freehead();
        free(entry);
        free(dir_node);
        free(dir);
        return fd;
      }
      free(entry);
    }
    free(dir_node);
    if(access == READ){
      printf("%s\n", "file does not found");
      free(path);
      free(dir);
      return EXIT_FAILURE;
    }else{
      printf("%s\n", "need to create this new file");
      free(path);
      free(dir);
      return EXIT_SUCCESS;
    }
  }

  // free(dir);
  // return EXIT_SUCCESS;
}

void set_permissions(permission_value old_value, permission_value *new_value) {
  old_value.owner = new_value->owner;
  old_value.group = new_value->group;
  old_value.others = new_value->others;
}

int f_write(void* buffer, int size, int ntimes, int fd ) {
  //check if the file accociated with this fd has been open
  superblock* sp = current_mounted_disk->superblock1;
  if (file_table[fd]->free_file == TRUE) {
    printf("%s\n", "The file must be open before write");
    return (EXIT_FAILURE);
  }
  if(file_table[fd]->access == READ){
    printf("%s\n", "File is not readable.");
    return EXIT_FAILURE;
  }
  if (file_table[fd]->file_inode->type == REG){
    printf("%s\n", "writing to a regular file");
    superblock *sp = current_mounted_disk->superblock1;
    //need to double check this
    void *datatowrite = malloc(size*ntimes);
    for(int j=0; j<ntimes; j++){
      memcpy(datatowrite+j*size, buffer,size);
    }
    int lefttowrite = size*ntimes;
    if (file_table[fd]->access == APPEND || file_table[fd]->access == READ ||file_table[fd]->access == READANDWRITE){
      //get the last data block of the file.
      void* last_data_block = malloc(BLOCKSIZE);
      void* copy = get_data_block(file_table[fd]->file_inode->last_block_index);
      memcpy(last_data_block, copy,BLOCKSIZE);
      free(copy);
      // file_table[fd]->byte_offset = file_table[fd]->file_inode->size;
      int offset_into_last_block = 0;
      int old_offset = 0;
      if (file_table[fd]->access == APPEND){
        offset_into_last_block = file_table[fd]->file_inode->size % BLOCKSIZE;
        old_offset = file_table[fd]->file_inode->size;
      }else{
        offset_into_last_block = file_table[fd]->byte_offset % BLOCKSIZE;
        old_offset = file_table[fd]->byte_offset;
      }
      int free_space = BLOCKSIZE - offset_into_last_block;
      if (free_space < sizeof(buffer) && sp->free_block == -1){
        printf("%s\n", "Not having enough space on the disk");
        free(datatowrite);
        free(last_data_block);
        return EXIT_FAILURE;
      }
      //TODO. complete the check of enough free space on disk
      int start_of_block_to_write = -1;
      int new_offset = old_offset + size*ntimes;
      int file_offset = old_offset;          int total_block = 0;
      if(free_space == 0){
        if (file_table[fd]->access == APPEND){
          ;
        }else{
          int block_written = file_table[fd]->byte_offset/BLOCKSIZE;
          total_block = block_written+1; // the block we are writing to in the future
        }

      }else{
        void* data = malloc(BLOCKSIZE);
        //copy the data from the last block to data1
        memcpy(data, last_data_block, offset_into_last_block);
        memcpy(data+offset_into_last_block, datatowrite, free_space);
        if(file_table[fd]->access == APPEND){
          write_data_to_block(file_table[fd]->file_inode->last_block_index, data, sp->size);
        }else{
          total_block = file_table[fd]->byte_offset/BLOCKSIZE;
          start_of_block_to_write = find_next_datablock(file_table[fd]->file_inode, total_block, old_offset, file_table[fd]->byte_offset);
          write_data_to_block(start_of_block_to_write, data, sp->size);
        }
        lefttowrite -= free_space;
        free(data);
        file_offset += free_space;
      }
      while (lefttowrite > 0) {
        if(file_table[fd]->access == APPEND){
          start_of_block_to_write = request_new_block();
        }else{
          start_of_block_to_write = find_next_datablock(file_table[fd]->file_inode, total_block+1, old_offset, file_table[fd]->byte_offset);
        }
        int size_to_write = BLOCKSIZE;
        void* data = malloc(BLOCKSIZE);
        if(lefttowrite < BLOCKSIZE){
          size_to_write = lefttowrite;
        }
        memcpy(data, datatowrite, size);
        write_data_to_block(start_of_block_to_write, data, size_to_write);
        lefttowrite -= size_to_write;
        file_offset += size_to_write;
        free(data);
      }
      file_table[fd]->byte_offset = new_offset;
      printf("new_offset: %d\n", new_offset);
      return size*ntimes;
    }
    //else if(file_table[fd]->access == WRITE || file_table[fd]->access == READANDWRITE){
    //     //need to overwite the file
    //     int start_block_index;
    //     int inode_num = file_table[fd]->byte_offset / BLOCKSIZE;
    //     int idtotal = N_IBLOCKS * BLOCKSIZE;
    //     int i2total = BLOCKSIZE * BLOCKSIZE;
    //     int i3total = i2total * BLOCKSIZE;
    //     if (inode_num < N_DBLOCKS){
    //       //update the dblocks
    //       start_block_index = file_table[fd]->file_inode->dblocks[inode_num];
    //     }else if(inode_num - N_DBLOCKS < idtotal){
    //       //update the idblocks.TODO
    //       int id_index = (inode_num - N_DBLOCKS)/BLOCKSIZE;//id_index <=4
    //
    //     }else if(inode_num - N_DBLOCKS - idtotal < i2total){
    //       //update the i2blocks.TODO
    //     }else if(inode_num - N_DBLOCKS- idtotal - i2total < i3total){
    //       //
    //     void* startplace_disk = get_data_block(start_block_index);
    //     int new_offset = ntimes*size;
    //     int old_file = file_table[td]->byte_offset;
    //     int offset = 0;   //offset in datatowrite
    //     //write to dblocks
    //     for(int i=0; i<N_DBLOCKS; i++){
    //       if (lefttowrite <= 0){
    //         return size*ntimes;
    //       }
    //       write_data_to_block(start_block_index, datatowrite, BLOCKSIZE);
    //       start_block_index = file_table[fd]->file_inode->dblocks[1];
    //       startplace_disk = get_data_block(start_block_index);
    //       lefttowrite -= sp->size;
    //       offset += sp->size;
    //       old_file -= BLOCKSIZE;
    //       if(old_file <= 0){
    //         //need to rewrite dblocks
    //       }
    //     }
    //     for(int i = 0; i <N_IBLOCKS; i++){
    //       if (lefttowrite <=0){
    //         return sizeof(buffer);
    //       }
    //     }
    //   }
    free(datatowrite);
    return sizeof(buffer);
  } else if (file_table[fd]->file_inode->type == DIR) {
    printf("%s\n", "writing to a dir file");
    //make_dir should do the same thing
  }
  return EXIT_SUCCESS;
}

boolean f_close(int file_descriptor) {
  if (file_descriptor >= FILETABLESIZE) {
    return FALSE;
  } else {
    file_table[file_descriptor]->free_file = TRUE;
    free(file_table[file_descriptor]->file_inode);
    table_freehead = find_next_freehead();
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

//TODO: finish this method and consider if offset can be negative??
boolean f_seek(int file_descriptor, int offset, int whence) {
  if(file_descriptor >=0 && file_descriptor < FILETABLESIZE) {
    if(offset < 0) {
      printf("Failure in fseek - invalid offset value.\n");
      return FALSE;
    } else {
      inode *file_inode = file_table[file_descriptor]->file_inode;
      int file_size = file_inode->size;

      if(whence == SEEKSET) {
        if(offset <= file_size) {
          file_table[file_descriptor]->byte_offset = offset;
          return TRUE;
        } else {
          printf("Failure in fseek - invalid offset value.\n");
          return FALSE;
        }
      } else if(whence == SEEKCUR) {
        if(file_table[file_descriptor]->byte_offset + offset <= file_size) {
          file_table[file_descriptor]->byte_offset += offset;
          return TRUE;
        }
      } else if (whence == SEEKEND) {
        return TRUE;
      } else {
        printf("Failure in f_seek - invalid whence value.\n");
        return FALSE;
      }
    }
  } else {
    printf("Failure in f_seek - invalid file descriptor.\n");
    return FALSE;
  }
  return FALSE;
}

boolean f_stat(char *filepath, stat *st) {
  permission_value *permissions = malloc(sizeof(permission_value));
  permissions->owner = '\a';
  permissions->group = '\a';
  permissions->others = '\a';
  int fd = f_open(filepath, READ, permissions);

  inode *inode1 = file_table[fd]->file_inode;
  st->size = inode1->size;
  st->uid = inode1->uid;
  st->gid = inode1->gid;
  st->ctime = inode1->ctime;
  st->mtime = inode1->mtime;
  st->atime = inode1->atime;
  st->type = inode1->type;
  st->permission.owner = inode1->permission.owner;
  st->permission.group = inode1->permission.group;
  st->permission.others = inode1->permission.others;
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
  printf("file permissions owner: %d\n", entry->permission.owner);
  printf("file permissions group: %d\n", entry->permission.group);
  printf("file permissions others: %d\n", entry->permission.others);
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

directory_entry* f_opendir(char* filepath) {
    //parse the filepath
    char path[strlen(filepath) + 1];
    char copy[strlen(filepath) + 1];
    strcpy(path, filepath);
    strcpy(copy, filepath);
    char *s = "/'";
    char *token;
    //calculate the level of depth
    token = strtok(copy, s);
    int count = 0;
    while (token != NULL) {
        count++;
        token = strtok(NULL, s);
    }

    //create directory_entry for the root
    // directory_entry* entry = malloc(sizeof(directory_entry));
    directory_entry *dir_entry = NULL;
    inode *root_node = get_inode(0);

    //add root directory to the open_file_table. Assume it is found.
    //if root already exists, should not add any more.

    if (file_table[0]->free_file == TRUE) {
        file_table_entry *root_table_entry = file_table[0];
        root_table_entry->free_file = FALSE;
        free(root_table_entry->file_inode);
        root_table_entry->file_inode = root_node;
        root_table_entry->byte_offset = 0;
        root_table_entry->access = READANDWRITE; //TODO: tell Rose about this...
    } else {
        free(root_node);
    }
    table_freehead = find_next_freehead();

    token = strtok(path, s);

    int i = 0;
    while (token != NULL && count >= 1) {
        //get the directory entry
        int found = FALSE;
        file_table[i]->byte_offset = 0;
        while (found == FALSE) {
            dir_entry = f_readdir(i);
            if (dir_entry == NULL) {
                //reach the last byte in the file
                break;
            }
            char *name = dir_entry->filename;
            if (strcmp(name, token) == 0) {
                printf("%s\n", "found");
                found = TRUE;
            }
            if (found == FALSE) free(dir_entry);
        }
        if (i != 0) {
            printf("%s\n", "here");
            //remove the ith index from the table. NEED CHECK. ROSE //TODO: ask her about this...
            // file_table[i] = NULL;
            // table_freehead = i;
            free(file_table[i]);
        } else {
            //root is always in the table. won't be removed.
            table_freehead = find_next_freehead();
            i = table_freehead;
        }
        if (found == FALSE) {
            printf("%s is not found\n", token);
            return NULL;
        }
        count--;
        //add dir_entry to the table
        //need to check if the dir_entry already exists
        inode *node = get_inode(dir_entry->inode_index);
        printf("already_in_table: %d\n", already_in_table(node));
        if (already_in_table(node) == -1) {
            file_table_entry *table_ent = file_table[i];
            table_ent->free_file = FALSE;
            free(table_ent->file_inode);
            table_ent->file_inode = node;
            table_ent->byte_offset = 0;
            table_ent->access = READANDWRITE; //TODO: tell Rose about this...
        } else {
            free(node);
        }
        //update table_freehead //TODO?
        token = strtok(NULL, s);
    }
    printf("%s\n", "end of open_dir-----");
    return dir_entry;
}

int get_table_freehead() {
    return table_freehead;
}

directory_entry* f_mkdir(char* filepath) {
    //filepath need to be converted to absolute path before hand
    char *newfolder = NULL;
    char *path = malloc(strlen(filepath));
    memset(path, 0, strlen(filepath));
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
    newfolder = strtok(path_copy, s);
    while (count > 1) {
        count--;
        path = strcat(path, "/");
        path = strcat(path, newfolder);
        newfolder = strtok(NULL, s);
    }
    printf("path: %s\n", path);
    printf("newfolder: %s\n", newfolder);
    directory_entry *dir = f_opendir(path);
    if (dir == NULL) {
        printf("cannot create directory. %s does not exists\n", path);
        free(path);
        free(dir);
        return NULL;
    } else {
        directory_entry *newf = malloc(sizeof(directory_entry));
        strcpy(newf->filename, newfolder);
        int new_inode_index = current_mounted_disk->superblock1->free_inode;
        if (new_inode_index == -1) {
            printf("%s\n", "not enough space to create new folder");
            free(newf);
            return NULL;
        }
        newf->inode_index = new_inode_index;
        //this node is the inode of parent dir
        inode *node = get_inode(dir->inode_index);
        inode *new_inode = get_inode(new_inode_index);
        current_mounted_disk->superblock1->free_inode = new_inode->next_inode;
        void *dir_data = get_data_block(node->last_block_index);
        if (node->size == BLOCKSIZE * (node->size / BLOCKSIZE) || node->size % BLOCKSIZE < sizeof(directory_entry)) {
            //request new blocks
            if (node->size % BLOCKSIZE < sizeof(directory_entry)) {
                //require padding first
                int padding_size = BLOCKSIZE - node->size % BLOCKSIZE;
                //Could be off by 1. CHECK. TODO
                memcpy(dir_data + node->size % BLOCKSIZE + 1, 0, padding_size);
                write_data_to_block(node->last_block_index, dir_data, BLOCKSIZE);
            }
            int new_block_index = request_new_block();
            void *content = malloc(BLOCKSIZE);
            memcpy(content, newf, sizeof(directory_entry));
            write_data_to_block(new_block_index, content, sizeof(directory_entry));
            //update superblock
            update_superblock_ondisk(current_mounted_disk->superblock1);
            //update inodes of parent dir
            //get inode_index
            int total_inode_num = node->size / BLOCKSIZE;
            node->size += sizeof(directory_entry);
            node->last_block_index = new_block_index;
            //TODO. dblock/idblock/i2/i3
            free(node);
            update_single_inode_ondisk(node, node->inode_index);
            //update inode for the new dir
            new_inode->size += sizeof(directory_entry);
            new_inode->type = DIR;
            if (new_inode->inode_index != new_inode_index) {
                printf("%s\n", "There is a problem.");
            }
            new_inode->dblocks[0] = new_block_index;
            update_single_inode_ondisk(new_inode, new_inode_index);
            return newf;
        } else {
            //append to the parent dir data block without padding
            int block_offset = node->size % BLOCKSIZE;
            memcpy(dir_data + block_offset, (void *) newf, sizeof(directory_entry));
            write_data_to_block(node->last_block_index, dir_data, BLOCKSIZE);
            //update parent dir inode
            node->size += sizeof(directory_entry);
            update_single_inode_ondisk(node, node->inode_index);
            return newf;
        }
    }
}

//TODO: update the time with the last accessed time, here!
int f_read(void *buffer, int size, int n_times, int file_descriptor) {
    if (file_descriptor < 0 || file_descriptor >= FILETABLESIZE) {
        printf("I am sorry, but the file descriptor is invalid.\n");
        return ERROR;
    }

    inode *file_to_read = file_table[file_descriptor]->file_inode;
    long file_offset = file_table[file_descriptor]->byte_offset;
    int bytes_remaining_in_file = file_to_read->size - file_offset;
    int block_to_read = file_offset / BLOCKSIZE;
    int bytes_to_read;
    int bytes_remaining_in_block;
    int block_offset;
    char *block_to_read_from;
    int access = file_table[file_descriptor]->access;

    if (access == APPEND || access == WRITE) {
        printf("I am sorry, but you are attempting to read a file opened for appending or writing. This is an invalid action.\n");
        return ERROR;
    }
    if (size <= 0 || n_times <= 0 || size > file_to_read->size || size * n_times > file_to_read->size ||
        size > bytes_remaining_in_file || size * n_times > bytes_remaining_in_file) {
        printf("I am sorry, but you are attempting to read an invalid number of bytes from the file.\n");
        return ERROR;
    }

    int buffer_index = 0;
    char *buffer_to_return = malloc(sizeof(size * n_times));
    int bytes_read = size * n_times;

    for (int i = 0; i < n_times; i++) {
        bytes_to_read = size;
        block_offset = file_offset % 512;
        while (size > 0) {
            int block_index = -1;
            block_to_read_from = get_block_from_index(block_to_read, file_to_read, &block_index);
            bytes_remaining_in_block = 512 - block_offset - 1;
            if (bytes_remaining_in_block >= size) {
                bytes_to_read = size;
                for (int j = 0; j < bytes_to_read; j++) {
                    buffer_to_return[buffer_index] = block_to_read_from[block_offset];
                    buffer_index++;
                    block_offset++;
                }
                size -= bytes_to_read;
                free_data_block(block_to_read_from);
            } else { //bytes_remaining_in_block < size
                bytes_to_read = bytes_remaining_in_block;
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

    return bytes_read;
}


int find_next_datablock(inode* inode, int total_block, int old_fileoffest, int current_offset) {
    //total_block: including the one we are writing to
    //current_offset: not including the bytes we are writing
    int idtotal = N_IBLOCKS * BLOCKSIZE / sizeof(int);
    int i2total = BLOCKSIZE * BLOCKSIZE / sizeof(int) / sizeof(int);
    int i3total = i2total * BLOCKSIZE / sizeof(int);
    int start_of_block_to_write = 0;
    if (current_offset >= old_fileoffest) {
        start_of_block_to_write = request_new_block();
        return start_of_block_to_write;
    }
    if (total_block < N_DBLOCKS) {
        start_of_block_to_write = inode->dblocks[total_block];
    } else if (total_block - N_DBLOCKS < idtotal) {
        int id_index = (total_block - N_DBLOCKS) / (BLOCKSIZE / sizeof(int));
        int data_offset = (total_block - N_DBLOCKS) % (BLOCKSIZE / sizeof(int));
        int data_index = inode->iblocks[id_index];
        void *data = get_data_block(data_index);
        start_of_block_to_write = *(int *) (data + data_offset * sizeof(int));
        free(data);
    } else if (total_block - N_DBLOCKS - idtotal < i2total) {
        int rest_block = total_block - N_DBLOCKS - idtotal;
        int data_offset1 = rest_block / (BLOCKSIZE / sizeof(int));
        void *data1 = get_data_block(inode->i2block);
        int data2_index = *(int *) (data1 + data_offset1 * sizeof(int));
        void *data2 = get_data_block(data2_index);
        int data_offset2 = (rest_block - (BLOCKSIZE / sizeof(int)) * data_offset1) % (BLOCKSIZE / sizeof(int));
        start_of_block_to_write = data_offset2;
        free(data2);
        free(data1);
    } else if (total_block - N_DBLOCKS - idtotal - i2total < i3total) {
        int rest_block = total_block - N_DBLOCKS - idtotal - i2total;
        void *data1 = get_data_block(inode->i3block);
        int data_offset1 = rest_block / (BLOCKSIZE / sizeof(int)) / (BLOCKSIZE / sizeof(int));
        int data2_index = *(int *) (data1 + (data_offset1) * sizeof(int));
        void *data2 = get_data_block(data2_index);
        rest_block = rest_block - data_offset1 * BLOCKSIZE / sizeof(int) * BLOCKSIZE / sizeof(int);
        int data_offset2 = rest_block / (BLOCKSIZE / sizeof(int));
        int data3_index = *(int *) (data2 + (data_offset2) * sizeof(int));
        void *data3 = get_data_block(data3_index);
        int data_offset3 = *(int *) (data3 + (data3_index) * sizeof(int));
        free(data3);
        free(data2);
        free(data1);
        start_of_block_to_write = data_offset3;
    }
    return start_of_block_to_write;
}


//TODO: pont define and figure out if you're requesting a block that doesn't exist
//TODO: check that this isn't just off by one
void *get_block_from_index(int block_index, inode *file_inode, int *data_region_index) { //block index is block overall to get...more computations must be done to know which block precisely to obtain
    //get direct block
    block *block_to_return = NULL;
    if (block_index >= 0 && block_index <= 9) {
        block_to_return = get_data_block(file_inode->dblocks[block_index]);
        *data_region_index = file_inode->dblocks[block_index];
    } else if (block_index >= 10 && block_index <= 521) {
        block_index -= 10; //adjust to array here
        int indirect_array_index = block_index / 128;
        int array_index = block_index % 128;
        int *indirect_block = get_data_block(file_inode->iblocks[indirect_array_index]);
        block_to_return = get_data_block(indirect_block[array_index]);
        *data_region_index = indirect_block[array_index];
        free_data_block(indirect_block);
    } else if (block_index >= 522 && block_index <= 16905) {
        block_index -= 522; //adjust to array here
        int indirect_array_index = block_index / 128;
        int array_index = block_index % 128;
        int *double_indirect_block = get_data_block(file_inode->i2block);
        int *indirect_block = get_data_block(double_indirect_block[indirect_array_index]);
        block_to_return = get_data_block(indirect_block[array_index]);
        *data_region_index = indirect_block[array_index];
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
        *data_region_index = indirect_block[array_index];
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
        superblock *superblock1 = current_mounted_disk->superblock1;
        if (directory_inode == NULL) {
            return FALSE;
        } else {
            //now, I have the inode for the file...and the disk's superblock! :)

            //TODO: Remove the file from its directory
            directory_entry *current_entry = f_readdir(index);
            directory_entry *directory_to_replace = NULL;
            // int byte_offset_of_directory_to_replace = 0;
            while (current_entry != NULL && strcmp(current_entry->filename, filename) != 0) {
                current_entry = f_readdir(index);
                // byte_offset_of_directory_to_replace += sizeof(directory_entry);
            }

            if (current_entry == NULL) {
                return FALSE; //wasn't able to find the entry in the directory
            } else {
                directory_to_replace = current_entry;
            }

            inode *file_inode = get_inode(directory_to_replace->inode_index);
            if (already_in_table(file_inode) != -1) {
                printf("I am sorry, you are attempting to remove a file that is open. Please close and try again!\n");
                return FALSE;
            }

            //get the last directory in the directory file and input it's information into this place
            int directory_file_size = directory_inode->size;
            int last_directory_byte_index = directory_file_size - sizeof(directory_entry);
            //check if the directory entry is the only one in the data block... (if so, then we need to reclaim the data block...)
            int block_to_fetch = last_directory_byte_index / BLOCKSIZE;

            directory_entry *directory_to_move = malloc(sizeof(directory_entry));
            int final_block_index = -1;
            void *data_block_containing_final_directory_entry = get_block_from_index(block_to_fetch, directory_inode,
                                                                                     &final_block_index);
            fseek(current_mounted_disk->disk_image_ptr, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK +
                                                        current_mounted_disk->superblock1->data_offset *
                                                        current_mounted_disk->superblock1->size +
                                                        final_block_index * BLOCKSIZE +
                                                        last_directory_byte_index % BLOCKSIZE, SEEK_SET);
            fread(directory_to_move, sizeof(directory_entry), 1, current_mounted_disk->disk_image_ptr);

            //need to write directory_to_replace back to the disk, now, since it has been deleted...
            directory_to_replace->inode_index = directory_to_move->inode_index;
            strcpy(directory_to_replace->filename, directory_to_move->filename);
            // fseek(); //TODO: replace with our fseek when needed
            file_table[index]->byte_offset -= sizeof(directory_entry); //pointer in file is now at the correct location to fwrite...
            int current_block_index = -1;
            int second_block_to_fetch = file_table[index]->byte_offset / BLOCKSIZE;
            void *data_block_containing_directory_to_replace = get_block_from_index(second_block_to_fetch,
                                                                                    directory_inode,
                                                                                    &current_block_index);
            fseek(current_mounted_disk->disk_image_ptr, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK +
                                                        current_mounted_disk->superblock1->data_offset *
                                                        current_mounted_disk->superblock1->size +
                                                        current_block_index * BLOCKSIZE +
                                                        file_table[index]->byte_offset % BLOCKSIZE, SEEK_SET);
            fwrite(directory_to_replace, sizeof(directory_entry), 1, current_mounted_disk->disk_image_ptr);

            if (block_to_fetch * BLOCKSIZE == last_directory_byte_index) {
                //this means that the directory entry to move is at the start of a directory file block...
                //TODO: update last block index here!!
                //TODO: decrease directory file size and free the final data block in the directory by adding it to the free list in the superblock...
                printf("block index to add to the free list %d\n", final_block_index);
                int *data_block = data_block_containing_final_directory_entry;
                data_block[0] = superblock1->free_block;
                superblock1->free_block = final_block_index;

                //write the data block to the disk and update the superblock
                fseek(current_mounted_disk->disk_image_ptr, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK +
                                                            current_mounted_disk->superblock1->data_offset *
                                                            current_mounted_disk->superblock1->size +
                                                            final_block_index * BLOCKSIZE, SEEK_SET);
                fwrite((void *) data_block_containing_final_directory_entry, BLOCKSIZE, 1,
                       current_mounted_disk->disk_image_ptr);
            } else {
                //this means that the directory entry to move is in the middle of a directory file block and so no reclaming is needed
            }

            free_data_block(data_block_containing_final_directory_entry);
            free_data_block(data_block_containing_directory_to_replace);

            directory_inode->size -= sizeof(directory_entry);
            fseek(current_mounted_disk->disk_image_ptr,
                  SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + superblock1->inode_offset * BLOCKSIZE +
                  directory_inode->inode_index * sizeof(inode), SEEK_SET);
            fwrite(directory_inode, sizeof(inode), 1, current_mounted_disk->disk_image_ptr);

            //TODO: Release the i-node to the pool of free inodes and re-write the value to the disk...
            //TODO: talk to Rose and see what else needs to be done here..
            int new_head = file_inode->inode_index;
            int old_head = superblock1->free_inode;

            //read in the current_head inode and write the new and correct value to disk...
            superblock1->free_inode = new_head;
            file_inode->next_inode = old_head;

            fseek(current_mounted_disk->disk_image_ptr,
                  SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + superblock1->inode_offset * BLOCKSIZE + new_head * sizeof(inode),
                  SEEK_SET);
            fwrite(file_inode, sizeof(inode), 1, current_mounted_disk->disk_image_ptr);

            //TODO: return the disk blocks to the pool of free disk blocks
            int file_size = file_inode->size;
            int file_blocks = ceilf((float) file_size / (float) BLOCKSIZE);
            void *block_to_replace;
            int block_value;

            for (int i = 0; i < file_blocks; i++) {
                block_to_replace = get_block_from_index(i, file_inode, &block_value);
                ((int *) block_to_replace)[0] = superblock1->free_block;
                superblock1->free_block = block_value;

                //write the data block to the disk and update the superblock
                fseek(current_mounted_disk->disk_image_ptr, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK +
                                                            current_mounted_disk->superblock1->data_offset *
                                                            current_mounted_disk->superblock1->size +
                                                            block_value * BLOCKSIZE, SEEK_SET);
                fwrite((void *) data_block_containing_final_directory_entry, BLOCKSIZE, 1,
                       current_mounted_disk->disk_image_ptr);
                free_data_block(block_to_replace);
            }
            //write the superblock to the disk...
            fseek(current_mounted_disk->disk_image_ptr, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK +
                                                        current_mounted_disk->superblock1->inode_offset *
                                                        current_mounted_disk->superblock1->size +
                                                        new_head * sizeof(inode), SEEK_SET);
            fwrite(superblock1, SIZEOFSUPERBLOCK, 1, current_mounted_disk->disk_image_ptr);

            //TODO: close the directory
            f_closedir(dir);
            return TRUE;
        }
    }
}

boolean f_closedir(directory_entry *entry) {
    if (entry == NULL) {
        return FALSE;
    } else {
        int inode_index = entry->inode_index;
        free(entry);
        int fd = -1;
        for (int i = 0; i < FILETABLESIZE; i++) {
            if (file_table[i]->file_inode->inode_index == inode_index) {
                //found the one you're searching for
                fd = i;
                break;
            }
        }

        if (fd == -1) {
            return FALSE;
        } else {
            return f_close(fd);
        }
    }
}

inode *get_inode_from_file_table_from_directory_entry(directory_entry *entry, int *table_index) {
    if (entry == NULL) {
        return NULL;
    } else {
        int inode_index = entry->inode_index;
        inode *directory_inode = NULL;
        for (int i = 0; i < FILETABLESIZE; i++) {
            if (!file_table[i]->free_file && file_table[i]->file_inode->inode_index == inode_index) {
                directory_inode = file_table[i]->file_inode;
                *table_index = i;
            }
        }
        return directory_inode;
    }
    return NULL;
}

directory_entry* f_readdir(int index_into_file_table) {
    if (index_into_file_table < 0 || index_into_file_table >= FILETABLESIZE) {
        printf("Index into the file table is invalid.\n");
        return NULL;
    }

    //fetch the inode of the directory to be read (assume the directory file is already open in the file table and that you are given the index into the file table)
    long offset_into_file = file_table[index_into_file_table]->byte_offset;
    inode *current_directory = file_table[index_into_file_table]->file_inode;
    if (current_directory->size - offset_into_file < sizeof(directory_entry)) {
        printf("Error! Attempting to read past the end of the directory file.\n");
        return NULL;
    }

    superblock *superblockPtr = current_mounted_disk->superblock1;
    long directory_index_in_block = offset_into_file / sizeof(directory_entry);
    long block = ((float) offset_into_file /
                  (float) superblockPtr->size);
    directory_entry *next_directory = malloc(sizeof(directory_entry));
    long offset_in_block = offset_into_file - (superblockPtr->size * block);
    long num_indirect = superblockPtr->size / sizeof(int);
    long num_directories = superblockPtr->size / sizeof(directory_entry);
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

int request_new_block() {
    superblock *sp = current_mounted_disk->superblock1;
    int free_block = sp->free_block;
    void *prevfree_block_on_disk = get_data_block(free_block);
    printf("%s\n", "+++++");
    int next_free = ((block *) prevfree_block_on_disk)->next_free_block;
    printf("free_block: %d\n", free_block);
    printf("newt_free: %d\n", next_free);
    sp->free_block = next_free;
    // if(update_superblock_ondisk(sp) != SIZEOFSUPERBLOCK){
    //   return EXIT_FAILURE;
    // }
    update_superblock_ondisk(sp);
    return free_block;
}

int update_superblock_ondisk(superblock* new_superblock) {
    FILE *current_disk = current_mounted_disk->disk_image_ptr;
    fseek(current_disk, SIZEOFBOOTBLOCK, SEEK_SET);
    // if (fwrite((void*)new_superblock, SIZEOFSUPERBLOCK, 1, current_disk) != SIZEOFSUPERBLOCK){
    //   printf("%s\n", "ERROR in update_superblock_ondisk()");
    //   return EXIT_FAILURE;
    // }
    fwrite((void *) new_superblock, SIZEOFSUPERBLOCK, 1, current_disk);
    return SIZEOFSUPERBLOCK;
}

int update_single_inode_ondisk(inode* new_inode, int new_inode_index) {
    FILE *current_disk = current_mounted_disk->disk_image_ptr;
    superblock *sp = current_mounted_disk->superblock1;
    int total_inode_num = (sp->data_offset - sp->inode_offset) * sp->size / sizeof(inode);
    if (new_inode_index > total_inode_num) {
        printf("%s\n", "NOT ENOUGH SPACE FOR INODES");
        return EXIT_FAILURE;
    }
    fseek(current_disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + sp->inode_offset * sp->size +
                        (new_inode->inode_index) * sizeof(inode), SEEK_SET);
    fwrite((void *) new_inode, sizeof(inode), 1, current_disk);
    return sizeof(new_inode);
}

//intended to write to data region
int write_data_to_block(int block_index, void* content, int size) {
    superblock *sp = current_mounted_disk->superblock1;
    FILE *current_disk = current_mounted_disk->disk_image_ptr;
    if (size > sp->size) {
        printf("%s\n", "Writing too much into one block.write_data_to_block()");
        return EXIT_FAILURE;
    }
    fseek(current_disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + sp->size * (sp->data_offset + block_index), SEEK_SET);
    // if(fwrite(content, size, 1, current_disk) == size){
    //     return size;
    // }
    fwrite(content, size, 1, current_disk);
    return EXIT_SUCCESS;
}

int already_in_table(inode* node) {
    int index2 = node->inode_index;
    for (int i = 0; i < FILETABLESIZE; i++) {
        if (file_table[i] != NULL) {
            if (file_table[i]->free_file == FALSE) {
                int index1 = file_table[i]->file_inode->inode_index;
                if (index1 == index2) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int find_next_freehead() {
    for (int i = 0; i < FILETABLESIZE; i++) {
        if (file_table[i]->free_file == TRUE) {
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
