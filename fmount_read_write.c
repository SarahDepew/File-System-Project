#include "filesystem.h"
#include "boolean.h"
#include "stdlib.h"

int main(){
  setup();
  boolean b = f_mount("DISKROOT","",0);
  printf("%d\n", b);
  shutdown();
}
