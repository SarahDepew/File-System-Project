#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  setup();
  f_mount("DISKDIR", "", 0);
  f_opendir("/a/s/d/sf/'we'/5/46/");
  shutdown();
}
