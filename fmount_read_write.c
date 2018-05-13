#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>

int main(){
  setup();
  int b = f_mount("DISKROOT","",0);
  // printf("%d\n", b);
  f_unmount(b); 
  shutdown();
}
