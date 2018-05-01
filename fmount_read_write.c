#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>

int main(){
  setup();
  f_mount("DISKROOT","",0);
  // printf("%d\n", b);
  shutdown();
}
