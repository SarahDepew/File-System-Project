//
// Created by Sarah Depew on 4/25/18.
//

#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    setup();
    f_mount("test1", "ignore");

    //create a basic disk image to use
    inode *inode1 = malloc(sizeof(inode));

    //set fields
    // inode1->filename[0] = 'T';
    // inode1->filename[1] = 'e';
    // inode1->filename[2] = 's';
    // inode1->filename[3] = 't';
    // inode1->filename[4] = '\0';
//     inode1->disk_identifier = 0;
//     inode1->parent_inode_index = -1;
//     inode1->next_inode = -1;
// //    inode1->f_stats = malloc(sizeof(stat));
//     inode1->size = 0; //TODO: figure this out later, hardcoding a reasonable size to be less than a block!
//     inode1->uid = 0;
//     inode1->gid = 0;
//     inode1->ctime = 0;
//     inode1->mtime = 0;
//     inode1->atime = 0;
//     inode1->type = REG;
//     inode1->permission = SUPER; //TODO: see how struct affects this?
//     inode1->inode_index = 0; //index in the inode region
//     inode1->dblocks[0] = 0;
//     // inode1->access = READ;
//     //dont need any other blocks, since this is one block sized
//
//     FILE *disk_image = fopen("Disk Image", "w+");
//
//     FILE *test_file = fopen("Test", "r+");
//     if(fseek(test_file, 0L, SEEK_END) == -1) {
//         perror("fseek failed.\n");
//         return FALSE;
//     }
//     long inputFileSize = ftell(test_file);
//     if(inputFileSize == -1) {
//         perror("File size error.\n");
//         return FALSE;
//     }
//     rewind(test_file);
//
//     //todo: read in block at a time!
//     void *allOfInputFile = malloc(inputFileSize);
//     if (allOfInputFile == NULL) {
//         perror("Malloc failed.\n");
//         return FALSE;
//     }
//
//     if(fread(allOfInputFile, inputFileSize, 1, test_file) < 1) {
//         perror("Error reading in disk image.\n");
//         return FALSE;
//     }
//     printf("%s\n", (char *) allOfInputFile);
//
//     inode1->size = inputFileSize;
//     fwrite(inode1, sizeof(inode), 1, disk_image); //TODO: binary write! (binary disk...)
//     fwrite(allOfInputFile, inputFileSize, 1, disk_image);
//     fclose(disk_image);
//
//     /* typedef struct cluster {
//         boolean read;
//         boolean write;
//         boolean execute;
//     } cluster;
//
//     //TODO: ask dianna about how to model this value? (byte operators)
//     typedef struct permission_value {
//         struct cluster cluster_owner;
//         struct cluster cluster_group;
//         struct cluster cluster_others;
//     } permission_value;
//     */

    //test f_open
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

    // f_mount("Disk Image", "point");
    // f_open("path", READ, permissions);

    return 0;
}
