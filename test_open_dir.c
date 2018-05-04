#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  setup();
  printf("%s\n", "-------------end set up--------");
  f_mount("DISKDIR", "", 0);
  printf("%s\n", "----------end f_mount---------");
  f_open("/a/d/s/4.txt", READ, NULL);
  printf("%s\n", "-----------end f_open1---------");
  f_open("/user/user.txt", READ, NULL);
  printf("%s\n", "------------end f_open2----------");
  // f_opendir("/a/s/d/sf/'we'/5/46/");
  shutdown();
}
