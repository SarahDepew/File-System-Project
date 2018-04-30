#include "filesystem.h"
#include "boolean.h"
#include "stdlib.h"

int main(){
  boolean b = f_mount("DISK","",0);
  printf("%d\n", b);
  
}
