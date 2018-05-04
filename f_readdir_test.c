//
// Created by Sarah Depew on 5/3/18.
//

#include "filesystem.h"
#include <stdlib.h>

//Test to open the formatted disk and read the entries in the root
int main() {
    // FILE *current_disk = fopen("Empty", "r");
    // void *data_block = malloc(sizeof(512));
    // fseek(current_disk, SIZEOFBOOTBLOCK + SIZEOFSUPERBLOCK + (12 * 512) + (0 * 512), SEEK_SET);
    // fread(data_block, 512, 1, current_disk);
    // directory_entry *directory_entry1 = data_block;
    // printf("%d\n", directory_entry1->inode_index);

   setup();

   int MTI = 0;
   f_mount("Empty", "N/A", &MTI);

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

   f_open("N/A", READ, permissions);

   directory_entry *value = f_readir(0);
   //
  //  printf("directory entry values:%d, %s\n", value->inode_index, value->filename);
   value = f_readir(0);
   printf("directory entry values:%d, %s\n", value->inode_index, value->filename);

   value = f_readir(0);
  //  printf("directory entry values:%d, %s\n", value->inode_index, value->filename);
   shutdown();
    return 0;
}
