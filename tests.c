#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "tests.h"
#include "filesystem.h"

#define CORRECTNUMARGUMENTS 2
#define FILELOCATION 1

void help() {
    printf("Use tests as follows: tests <name of disk to test>\n");
}

void check_freehead() {
    int expected_freehead = desired_free_location_in_table(1);
    assert(expected_freehead == get_table_freehead());
}

//Pass in the name of the disk to be tested
int main(int argc, char *argv[]) {
    if (argc != CORRECTNUMARGUMENTS) {
        help();
    } else {
        run_tests(argv[FILELOCATION]);
    }
}

void run_tests(char *disk_to_test) {
    printf("*************Setting up filesystem*************\n");
    setup();
    printf("*************Done setting up filesystem*************\n");

//    printf("*************Testing f_mount*************\n");
//    test_fmount(disk_to_test);
//    printf("*************Done Testing f_mount*************\n");
//
//    printf("*************Testing f_unmount*************\n");
//    test_funmount(disk_to_test);
//    printf("*************Done Testing f_unmount*************\n");

    // printf("*************Testing f_open*************\n");
    // printf("Testing f_open on a file that does not exist with a valid path and writing/appending as the value.\n");
    //
    //
    // printf("*************Done Testing f_open*************\n");


    printf("*************Testing f_opendir*************\n");
    test_fopendir_root(disk_to_test, "/");
    printf("%s\n", "before invalid");
    test_fopendir_invalid(disk_to_test, "/helloworld");
    printf("*************Done Testing f_opendir*************\n");

    printf("*************Testing f_readdir*************\n");
    test_freaddir_root(disk_to_test);
    printf("*************Done Testing f_readdir*************\n");

    printf("*************Shutting down filesystem*************\n");
    shutdown();
    printf("*************Done shutting down filesystem*************\n");
}

//1) Check that the values mounted are as expected for the given disk
void test_fmount(char *disk_to_mount) {
    int expected_mid = first_free_location_in_mount();
    FILE *disk = fopen(disk_to_mount, "rb");
    superblock *expectedSuperblock = malloc(SIZEOFSUPERBLOCK);
    fseek(disk, SIZEOFBOOTBLOCK, SEEK_SET);
    fread(expectedSuperblock, SIZEOFSUPERBLOCK, 1, disk);

    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);
    assert(mid == expected_mid);
    mount_table_entry *table_entry = get_mount_table_entry(mid);
    assert(table_entry->free_spot == FALSE);
    assert(table_entry->disk_image_ptr != NULL);
    assert(table_entry->superblock1 != NULL);

    superblock *actualSuperblock = table_entry->superblock1;
    assert(actualSuperblock->size == expectedSuperblock->size);
    assert(actualSuperblock->inode_offset == expectedSuperblock->inode_offset);
    assert(actualSuperblock->data_offset == expectedSuperblock->data_offset);
    assert(actualSuperblock->free_inode == expectedSuperblock->free_inode);
    assert(actualSuperblock->free_block == expectedSuperblock->free_block);
    assert(actualSuperblock->root_dir == expectedSuperblock->root_dir);

    free(expectedSuperblock);
    fclose(disk);
    f_unmount(mid);
}

//1) First mount a disk and then check that the spot in the mounted disks array is marked as free and that there are no memory leaks
void test_funmount(char *disk_to_mount) {
    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);
    f_unmount(mid);
    mount_table_entry *table_entry = get_mount_table_entry(mid);
    assert(table_entry->free_spot == TRUE);
}

//1) test fopen on a file that does not exist with a valid path and writing/appending as the value - expected behavior is that the file is created and the file is added to the directory as well as opened in the file table
void test_fopen_create(char *filepath, int access, permission_value *permissions) {
    int expected_fd = desired_free_location_in_table(2);
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

    free(path);

    //check the file is added to the file table in the correct location
    int fd = f_open(filepath, access, permissions);
    assert(fd == expected_fd);

    //check that the expected inode is given to the file
    entry = get_table_entry(fd);
    assert(entry->file_inode->inode_index == expected_inode);
    assert(entry->free_file == FALSE);
    assert(entry->byte_offset == 0);
    assert(entry->access == access);

    //check that the filename is added to the directory with the correct inode index
    int parent_inode_index = entry->file_inode->parent_inode_index;
    int fd_parent_dir = get_fd_from_inode_value(parent_inode_index);
    directory_entry dir_entry = get_last_directory_entry(fd_parent_dir);
    assert(dir_entry.inode_index == expected_inode);
    assert(strcmp(dir_entry.filename, filename) == 0);

    check_freehead();

    f_close(fd_parent_dir);
    f_close(fd);
}


/* Testing f_opendir */
//1)Test a valid path
void test_fopendir_valid(char *disk_to_mount, char* filepath) {
    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);

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

    filename = strtok(path_copy, s);
    while (count > 1) {
        count--;
        path = strcat(path, "/");
        path = strcat(path, filename);
        filename = strtok(NULL, s);
    }

    free(path);

    int expected_tid = desired_free_location_in_table(1);

    directory_entry *directory_entry1 = f_opendir(filepath);
    assert(directory_entry1 != NULL);
    assert(expected_tid == get_fd_from_inode_value(directory_entry1->inode_index));
    assert(directory_entry1->filename == filename);

    file_table_entry *entry = get_table_entry(expected_tid);
    assert(entry->free_file == FALSE);
    assert(entry->file_inode->inode_index == directory_entry1->inode_index);
    assert(entry->byte_offset == 0);
    assert(entry->file_inode->type == DIR);

    check_freehead();
    f_closedir(directory_entry1);
    f_unmount(mid);
}

//2)Test an invalid path
void test_fopendir_invalid(char *disk_to_mount, char* filepath) {
    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);

    directory_entry *directory_entry1 = f_opendir(filepath);
    assert(directory_entry1 == NULL);

    check_freehead();
    f_unmount(mid);
}

//3)Test opening root dir only
void test_fopendir_root(char *disk_to_mount, char* filepath) {
    int expected_inode = 0;
    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);

    int expected_tid = 0;
    file_table_entry *entry = get_table_entry(expected_tid);
    assert(entry->free_file == TRUE);
    directory_entry *directory_entry1 = f_opendir(filepath);
    assert(expected_tid == get_fd_from_inode_value(directory_entry1->inode_index));

    //TODO: uncomment once not NULL
//    assert(directory_entry1 != NULL)
//    assert(directory_entry1->inode_index == expected_inode);
//    assert(strcmp(directory_entry1->filename, "/") == 0);

    //check that the expected inode is given to the file
    entry = get_table_entry(expected_tid);
    assert(entry->file_inode->inode_index == expected_inode);
    assert(entry->file_inode->type == DIR); //check a directory was opened
    assert(entry->free_file == FALSE);
    assert(entry->byte_offset == 0);
    assert(entry->access == READANDWRITE);

    check_freehead();

    //TODO: uncomment
//    f_closedir(directory_entry1);
//    check_freehead();

    f_unmount(mid);
}

//4)Test opening a directory that is already opened
void test_fopendir_alread_open(char *disk_to_mount, char* filepath) {
    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);

    directory_entry *directory_entry1 = f_opendir(filepath);
    assert(directory_entry1 != NULL);

    directory_entry *directory_entry2 = f_opendir(filepath);
    assert(directory_entry1 == NULL);

    check_freehead();
    f_closedir(directory_entry1);
    f_unmount(mid);
}

/* Testing f_readdir */
//1) Open root and check the root inode's values (should be . and .. at beginning)
void test_freaddir_root(char *disk_to_mount) {
    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);
    directory_entry *return_from_opendir = f_opendir("/");
    int expected_fd = 0;

    int fd = get_fd_from_inode_value(return_from_opendir->inode_index);
    assert(fd == expected_fd);

    directory_entry *entry = f_readdir(fd);
    
    assert(entry->inode_index == 0);
    assert(strcmp(entry->filename, ".") == 0);

    entry = f_readdir(fd);
    assert(entry->inode_index == 0);
    assert(strcmp(entry->filename, "..") == 0);

    //TODO: uncomment once not null
//    f_closedir(return_from_opendir);

    f_unmount(mid);
}

//3) Test reading past the end of a file
void test_freaddir_past_end(char *disk_to_mount, char *filepath) {
    int mid = -1;
    f_mount(disk_to_mount, "N/A", &mid);
    int expected_fd = desired_free_location_in_table(1);

    directory_entry *return_from_opendir = f_opendir(filepath);
    assert(expected_fd == get_fd_from_inode_value(return_from_opendir->inode_index));

    int file_size = get_table_entry(get_fd_from_inode_value(return_from_opendir->inode_index))->file_inode->size;
    int num_directory_entries = file_size/sizeof(directory_entry);
    assert(num_directory_entries*sizeof(directory_entry) == file_size);
    directory_entry *entry = NULL;

    for(int i=0; i<=num_directory_entries; i++) {
        entry = f_readdir(expected_fd);
    }



    f_closedir(return_from_opendir);
    f_unmount(mid);
}
