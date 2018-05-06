#include <stdio.h>
#include "tests.h"
#include "filesystem.h"

#define CORRECTNUMARGUMENTS 2
#define FILELOCATION 1

void help() {
    printf("Use tests as follows: tests <name of disk to test>\n");
}

//Pass in the name of the disk to be tested
int main(int argc, char *argv[]) {
    if(argc != CORRECTNUMARGUMENTS) {
        help();
    } else {
        run_tests(argv[FILELOCATION]);
    }
}

void run_tests(char *disk_to_test) {
    printf("*************Testing f_mount*************\n");
    test_fmount(disk_to_test);
    printf("*************Done Testing f_mount*************\n");

    printf("*************Testing f_open*************\n");
    printf("Testing f_open on a file that does not exist with a valid path and writing/appending as the value.\n");


    printf("*************Done Testing f_open*************\n");
}

//1) Check that the values mounted are as expected for the given disk
void test_fmount(char *disk_to_mount){
    int expected_mid = first_free_location_in_mount();
    FILE *disk = fopen(disk_to_mount, "rb");
    superblock *expectedSuperblock = malloc(SIZEOFSUPERBLOCK);
    fseek(disk, SIZEOFBOOTBLOCK, SEEK_SET);
    fread(superblock1, SIZEOFSUPERBLOCK, 1, disk);

    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);
    mount_table_entry table_entry = get_mount_table_entry();
    assert(table_entry.free_spot == FALSE);
    assert(table_entry.disk_image_ptr != NULL);
    assert(table_entry.superblock1!=NULL);

    superblock *actualSuperblock = table_entry->superblock1;
    assert(actualSuperblock->size == expectedSuperblock->size);
    assert(actualSuperblock->inode_offset == expectedSuperblock->size);
    assert(actualSuperblock->data_offset == expectedSuperblock->data_offset);
    assert(actualSuperblock->free_inode == expectedSuperblock->free_inode);
    assert(actualSuperblock->free_block == expectedSuperblock->free_block);
    assert(actualSuperblock->root_dir == expectedSuperblock->root_dir);

    free(expectedSuperblock);
    f_unmount(mid);
}

//1) test fopen on a file that does not exist with a valid path and writing/appending as the value - expected behavior is that the file is created and the file is added to the directory as well as opened in the file table
void test_fopen_create(char *filepath, int access, permission_value *permissions) {
    int expected_fd = second_free_location_in_table();
    int expected_inode = first_free_inode();
    file_table_entry *entry = NULL;

    //get the filename
    char *filename = NULL;
    char *path = malloc(strlen(filepath));
    memset(path, 0, strlen(filepath));
    char path_copy[strlen(filepath) + 1];
    char copy[strlen(filepath) + 1];
    strcpy(path_copy, filepath);
    strcpy(copy, filepath);
    char *s = "/'";
    //calculate the level of depth of dir
    char *token = strtok(copy, s);
    int count = 0;
    while (token != NULL) {
        count++;
        token = strtok(NULL, s);
    }
    // printf("count : %d\n", count);
    filename = strtok(path_copy, s);
    while (count > 1) {
        count--;
        path = strcat(path, "/");
        path = strcat(path, filename);
        filename = strtok(NULL, s);
    }
    printf("path: %s\n", path);
    printf("filename: %s\n", filename);
    free(path);

    //check the file is added to the file table in the correct location
    int fd = f_open(filepath, access, permissions);
    assert(fd == expected_fd);

    //check that the expected inode is given to the file
    entry = get_table_entry(fd);
    assert(entry->file_inode->inode_index, expected_inode);
    assert(entry->free_file == FALSE);
    assert(entry->byte_offset == 0);
    assert(entry->access == access);

    //check that the filename is added to the directory with the correct inode index
    int parent_inode_index = entry->file_inode->parent_inode_index;
    int fd_parent_dir = get_fd_from_inode_value(parent_inode_index);
    directory_entry *entry = get_last_directory_entry(fd_parent_dir);
    assert(entry->inode_index == expected_inode);
    assert(strcmp(entry->filename, filename) == 0);

    f_close(fd_parent_dir);
    f_close(fd);
}
