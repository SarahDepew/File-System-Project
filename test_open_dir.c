#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  // FILE *disk = fopen("DISKDIR", "r");
  // inode *block = malloc(BLOCKSIZE);
  // fseek(disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + 2 * sizeof(inode), SEEK_SET);
  // fread(block, sizeof(inode), 1, disk);

  setup();
  printf("%s\n", "-------------end set up--------");
  f_mount("DISKDIR", "", 0);
  printf("%s\n", "----------end f_mount---------");
  f_open("/a/d/s/4.txt", READ, NULL);
  print_file_table();
  printf("%s\n", "-----------end f_open1---------");
  f_open("/user/user.txt", READ, NULL);
  printf("%s\n", "------------end f_open2----------");
  int fd = f_open("/user/test.txt", APPEND, NULL);
  printf("%s\n", "------------end f_open3----------");
  // f_opendir("/a/s/d/sf/'we'/5/46/");
  printf("fd: %d\n", fd);
  char* content = "1";
  char* large = "bootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootbootboot";
  f_write(content, 1, 1, fd);
  void *block = get_data_block(2);
  printf("Contents of block2: %s\n", (char*) block);
  printf("%s\n", "----------done writing 1----------");
  f_write(large, strlen(large), 1, fd);
  block = get_data_block(3);
  printf("Contents of block3: %s\n", (char*) block);
  inode* node = get_inode(2);
  print_inode(node);
  // f_stat("/user/test.txt", stats);
  // stat *stats = malloc(sizeof(stat));
  // printf("stats values %d, %d\n", stats->size, stats->inode_index);
  // f_write(text, strlen(text), 1, fd);
  // void* buffer = malloc(200);
  // f_read(buffer, 5, 1, 1);
  // inode *i = get_inode(2);
  // printf("%s\n", (char*) buffer);
  shutdown();
}
