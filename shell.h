//
// Created by Sarah Depew on 5/7/18.
//

#ifndef SHELL_H_INCLUDED
#define SHELL_H_INCLUDED

#include <zconf.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boolean.h"
#include "parser.h"
#include "filesystem.h"
#include <termios.h>

#define BUFFERSIZE 4096
#define RUNNING 0
#define SUSPENDED 1
#define KILLED 2
#define FALSE 0
#define TRUE 1
#define NUMBER_OF_BUILT_IN_FUNCTIONS 16
#define NUMBER_OF_VALID_USERS 2
#define NOT_FOUND -1

int EXIT;
int shell; // shell fd
pid_t shell_pgid; // shell process group
typedef int (*operation)(char**);

//Structs
typedef struct builtin {
    char *tag;
    operation function;
} builtin;

typedef struct tokenizer {
    char *str;
    char *pos;
} tokenizer;

//we can add more flags for future command expansion
typedef struct process {
    struct process *next_process;
    pid_t pid;
    char **args;
    int status;
} process;

typedef struct job {
    struct process *first_process;
    pid_t pgid;
    char *job_string;
    char *full_job_string;
    int status;
    int pass;
    struct termios termios_modes;
    int stdin;
    int stdout;
    char **args;
    int stderr;
    int run_in_background;
    int suspend_this_job;
    struct job *next_job;
} job;

typedef struct background_job {
    struct background_job *next_background_job;
    pid_t pgid;
    int status;
    char *job_string;
    int verbose;
    struct termios termios_modes;
} background_job;

typedef struct user {
    char *name;
    char *password;
    int uid;
    char *absolute_path_home_directory;
} user;

void printoutargs();

background_job *get_background_from_pgid(pid_t pgid);

int lengthOf(char *str);

int arrayLength(char **array);

/* Make sure that the filesystem is set up */
void initializeFilesystem();

/* Make sure that the file system's memory is freed */
void shutdownFilesystem();

/* make structs for built in commands */
void buildBuiltIns();

/* Create the array of valid users */
void create_users();

/* Login process for shell */
void login();

/* Displays error message */
void printError(char* message);

/* Frees commands and displays error message */
void handleError(char* message, char **commands, int numCommands);

/* Method that takes a command pointer and checks if the command is a background or foreground job.
 * This method returns 0 if foreground and 1 if background */
int isBackgroundJob(job* job1);

/* child process has terminated and so we need to remove the process from the linked list (by pid).
 * We would call this function in the signal handler when getting a SIGCHLD signal. */
void childReturning(int sig, siginfo_t *siginfo, void *context);

/* This method is simply the remove node method called when a node needs to be removed from the list of jobs. */
void removeNode(pid_t pidToRemove);

/* Passes in the command to check. Returns the index of the built-in command if it’s in the array of built-in commands and -1 if it is not in the array allBuiltIns */
int isBuiltInCommand(process cmd);

/* Compares a process and a built in by name and determines if they are equal or not. */
int process_equals(process process1, builtin builtin1);

/* Passes in the built-in command to be executed along with the index of the command in the allBuiltIns array. This method returns true upon success and false upon failure/error. */
int executeBuiltInCommand(process *process1, int index);

void free_background_jobs();

/* Method to launch our process in either the foreground or the background. */
//void launchProcess(process *process1, pid_t pgid, int foreground);

void launchJob (job *j, int foreground);
void launchProcess (process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground);
void put_job_in_foreground (job *j, int cont);
void put_job_in_background (job *j, int cont, int status);
void background_built_in_helper(background_job *bj, int cont, int status);

/* Method to make sure the shell is running interactively as the foreground job before proceeding. Modeled after method found on https://www.gnu.org/software/libc/manual/html_mono/libc.html#Foreground-and-Background. */
void initializeShell();

/* iterates through LL and returns true if there are >= nodes as pnum */
int processExists(int pnum);

/*Method that exits the shell. This command sets the global EXIT variable that breaks out of the while loop in the main function.*/
int exit_builtin(char** args);

/* Method to iterate through the linked list and print out node parameters. */
int jobs_builtin(char** args);

/* Method to take a job id and send a SIGTERM to terminate the process.*/
int kill_builtin(char** args);

/* Method that sends continue signal to suspended process in background -- this is bg*/
int background_builtin(char** args);

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground_builtin(char** args);

int ls_builtin(char **args);
int chmod_builtin(char **args);
int mkdir_builtin(char **args);
int rmdir_builtin(char **args);
int cd_builtin(char **args);
int pwd_builtin(char **args);
int cat_builitn(char **args);
int more_builtin(char **args);
int rm_builtin(char **args);
int mount_builtin(char **args);
int unmount_builtin(char **args);
void errorMessage();
void print_args(char **args);

directory_entry* goto_root();
directory_entry* goto_destination(char* filepath);
char* convert_absolute(char* filepath);
#endif //HW7_SHELL_H
