#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  setup();
  printf("%s\n", "-------------end set up--------");
  f_mount("DISKDIR", "", 0);
  printf("%s\n", "----------end f_mount---------");
  f_open("/a/d/s/4.txt", READ, NULL);
  // print_file_table();
  printf("%s\n", "-----------end f_open1---------");
  f_open("/user/user.txt", READ, NULL);
  printf("%s\n", "------------end f_open2----------");
  int fd = f_open("/user/test.txt", READ, NULL);
  printf("%s\n", "------------end f_open3----------");
  // f_opendir("/a/s/d/sf/'we'/5/46/");
  // printf("fd: %d\n", fd);
  // void* buffer = malloc(200);
  // f_read(buffer, 18, 1, fd);
  // printf("%s\n", (char*) buffer);
  printf("%s\n", "-------------end of fread---------");
  // f_write();
  // free(buffer);
  shutdown();
}
