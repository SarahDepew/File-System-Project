#include "shell.h"
#include "parser.h"
#include "filesystem.h"
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

job *all_jobs;
job *list_of_jobs = NULL;
background_job *all_background_jobs = NULL;
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

//strings for built in functions
char *exit_string = "exit\0";
char *kill_string = "kill\0";
char *jobs_string = "jobs\0";
char *fg_string = "fg\0";
char *bg_string = "bg\0";
char *ls_string = "ls\0";
char *chmod_string = "chmod\0";
char *mkdir_string = "mkdir\0";
char *rmdir_string = "rmdir\0";
char *cd_string = "cd\0";
char *pwd_string = "pwd\0";
char *cat_string = "cat\0";
char *more_string = "more\0";
char *rm_string = "rm\0";
char *mount_string = "mount\0";
char *unmount_string = "unmount\0";

//TODO: add flags to the parser
//flags
char *F = "-F\0";
char *l = "-l\0";


//strings and variables for jobs printout
char *running = "Running\0";
char *suspended = "Suspended\0";
char *killed = "Killed\0";

char *builtInTags[NUMBER_OF_BUILT_IN_FUNCTIONS];
struct builtin allBuiltIns[NUMBER_OF_BUILT_IN_FUNCTIONS];

int root_index_into_mount_table = -1;
directory_entry *pwd_directory = NULL;
int root_directory_ft_entry = 0;

user *current_user;
user *valid_users[NUMBER_OF_VALID_USERS];

/* Main method and body of the function. */
int main (int argc, char **argv) {
    EXIT = FALSE;

    initializeFilesystem(-1);
    initializeShell();
    buildBuiltIns(); //store all builtins in built in array

    //mount filesystem and open root
    if (f_mount("DISK", "N/A", &root_index_into_mount_table) == FALSE) { //TODO: change back to DISK!
        EXIT = TRUE;
    }

    //ask the user to log into the shell
    create_users();
    login();

    pwd_directory = f_opendir(current_user->absolute_path_home_directory);

    while (!EXIT) {

        perform_parse();
        job *currentJob = all_jobs;

        /* run list of jobs entered */
        while (currentJob != NULL) {
            if (!(currentJob->pass)) {
                launchJob(currentJob, !(currentJob->run_in_background));
            }
            currentJob = currentJob->next_job;
        }

        /* add job to background if it is flagged */
        job *to_suspend = all_jobs;
        while (to_suspend != NULL) {
            if (to_suspend->suspend_this_job) {
                put_job_in_background(to_suspend, 0, SUSPENDED);
            }
            to_suspend = to_suspend->next_job;
        }

        /*Print out job status updates */
        background_job *bj = all_background_jobs;
        int i = 0;
        while (bj != NULL) {
            char *status[] = {running, suspended, killed};
            i++;
            if (bj->verbose) {
                printf("\n[%d] %s \t\t %s\n", i, status[bj->status], bj->job_string);
                bj->verbose = FALSE;
            }
            bj = bj->next_background_job;

        }

        free_all_jobs();

    }

    free(pwd_directory);
    f_unmount(root_index_into_mount_table);
    shutdownFilesystem();

    /* free any background jobs still in LL before exiting */
    free_background_jobs();
    return EXIT_SUCCESS;
}

/* Make sure that the filesystem is set up */
void initializeFilesystem(mid) {
    setup();
    f_mount("DISKDIR", "N/A", &mid);
}

/* Make sure that the file system's memory is freed */
void shutdownFilesystem(int mid) {
    f_unmount(mid);
    shutdown();
}

/* Make sure the shell is running interactively as the foreground job
   before proceeding. */ // modeled after https://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html
void initializeShell() {
    /* See if we are running interactively.  */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        /* Loop until we are in the foreground.  */
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        /* Ignore interactive and job-control signals.  */
        if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
            perror("I am sorry, but signal failed.\n");
            exit(EXIT_FAILURE);
        }
        if (signal(SIGTERM, SIG_IGN) == SIG_ERR) {
            perror("I am sorry, but signal failed.\n");
            exit(EXIT_FAILURE);
        }
        if (signal(SIGQUIT, SIG_IGN) == SIG_ERR) {
            perror("I am sorry, but signal failed.\n");
            exit(EXIT_FAILURE);
        }
        if (signal(SIGTTIN, SIG_IGN) == SIG_ERR) {
            perror("I am sorry, but signal failed.\n");
            exit(EXIT_FAILURE);
        }
        if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
            perror("I am sorry, but signal failed.\n");
            exit(EXIT_FAILURE);
        }
        if (signal(SIGTSTP, SIG_IGN) == SIG_ERR) {
            perror("I am sorry, but signal failed.\n");
            exit(EXIT_FAILURE);
        }

        /* registering sigchild handler */
        struct sigaction childreturn;
        memset(&childreturn, 0, sizeof(childreturn));
        childreturn.sa_sigaction = &childReturning;
        sigset_t mask;
        sigemptyset (&mask);
        sigaddset(&mask, SIGCHLD);
        childreturn.sa_mask = mask;
        /* add sig set for sig child and sigtstp */
        childreturn.sa_flags = SA_SIGINFO | SA_RESTART;
        if (sigaction(SIGCHLD, &childreturn, NULL) < 0) {
            perror("Error with sigaction for child.\n");
            return;
        }

        /* Put ourselves in our own process group.  */
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group.\n");
            exit(1);
        }

        /* Grab control of the terminal.  */
        tcsetpgrp(shell_terminal, shell_pgid);

        /* Save default terminal attributes for shell.  */
        tcgetattr(shell_terminal, &shell_tmodes);
    } else {
        perror("I am sorry, there was an error with initializing the shell.\n");
        exit(EXIT_FAILURE);
    }
}

/* Make array for built in commands. */
void buildBuiltIns() {
    char *builtInTags[NUMBER_OF_BUILT_IN_FUNCTIONS] = {exit_string, kill_string, jobs_string, fg_string, bg_string,
                                                       ls_string,
                                                       chmod_string, mkdir_string, rmdir_string, cd_string, pwd_string,
                                                       cat_string, more_string, rm_string, mount_string,
                                                       unmount_string};

    for (int i = 0; i < NUMBER_OF_BUILT_IN_FUNCTIONS; i++) {
        allBuiltIns[i].tag = builtInTags[i];

        if (i == 0) {
            allBuiltIns[i].function = exit_builtin;
        } else if (i == 1) {
            allBuiltIns[i].function = kill_builtin;
        } else if (i == 2) {
            allBuiltIns[i].function = jobs_builtin;
        } else if (i == 3) {
            allBuiltIns[i].function = foreground_builtin;
        } else if (i == 4) {
            allBuiltIns[i].function = background_builtin;
        } else if (i == 5) {
            allBuiltIns[i].function = ls_builtin;
        } else if (i == 6) {
            allBuiltIns[i].function = chmod_builtin;
        } else if (i == 7) {
            allBuiltIns[i].function = mkdir_builtin;
        } else if (i == 8) {
            allBuiltIns[i].function = rmdir_builtin;
        } else if (i == 9) {
            allBuiltIns[i].function = cd_builtin;
        } else if (i == 10) {
            allBuiltIns[i].function = pwd_builtin;
        } else if (i == 11) {
            allBuiltIns[i].function = cat_builtin;
        } else if (i == 12) {
            allBuiltIns[i].function = more_builtin;
        } else if (i == 13) {
            allBuiltIns[i].function = rm_builtin;
        } else if (i == 14) {
            allBuiltIns[i].function = mount_builtin;
        } else {
            allBuiltIns[i].function = unmount_builtin;
        }
    }
}

void create_users() {
    user *super_user = malloc(sizeof(user));
    user *basic_user = malloc(sizeof(user));

    super_user->uid = 0;
    basic_user->uid = 1;
    //TODO: add home directories for users to the disk created in format...
    super_user->absolute_path_home_directory = "/";
    basic_user->absolute_path_home_directory = "/";
    super_user->name = "super\n";
    basic_user->name = "basic\n";
    super_user->password = "suser\n";
    basic_user->password = "buser\n";

    valid_users[0] = super_user;
    valid_users[1] = basic_user;
}

void login() {
    boolean login_valid = FALSE;
    char *buffer = malloc(BUFFERSIZE);

    while (!login_valid) {
        printf("Please enter a username less than 80 characters(super or basic): ");
        memset(buffer, 0, BUFFERSIZE);
        fgets(buffer, sizeof(buffer), stdin);
        if (strcmp(valid_users[0]->name, buffer) != 0 && strcmp(valid_users[1]->name, buffer) != 0) {
            printf("Invalid username: %s. Please try again.\n", buffer);
            continue;
        }
        //password: suser buser
        printf("Please enter a password: ");
        memset(buffer, 0, BUFFERSIZE);
        fgets(buffer, sizeof(buffer), stdin);
        if (strcmp(valid_users[0]->password, buffer) != 0 && strcmp(valid_users[1]->password, buffer) != 0) {
            printf("Invalid password: %s. Please try again.\n", buffer);
            continue;
        }

        if (strcmp(valid_users[0]->password, buffer) == 0) {
            current_user = valid_users[0];
        } else {
            current_user = valid_users[1];
        }

        free(buffer);
        login_valid = TRUE;
    }
}

/* Method that takes a command pointer and checks if the command is a background or foreground job.
 * This method returns 0 if foreground and 1 if background */
int isBackgroundJob(job* job1) {
    return job1->run_in_background == TRUE;
}

/* takes pgid to remove and removes corresponding pgid, if it exists */
void trim_background_process_list(pid_t pid_to_remove) {
    background_job *cur_background_job = all_background_jobs;
    background_job *prev_background_job = NULL;
    while (cur_background_job != NULL) {
        if (cur_background_job->pgid == pid_to_remove) { //todo: check this code, since I don't think it works correctly...
            if (prev_background_job == NULL) {
                background_job *temp = cur_background_job->next_background_job;
                all_background_jobs = temp;
                free(cur_background_job->job_string);
                free(cur_background_job);
                return;
            }
            else {
                prev_background_job->next_background_job = cur_background_job->next_background_job;
                free(cur_background_job->job_string);
                free(cur_background_job);
                return;
            }
        }
        else{
            prev_background_job = cur_background_job;
            cur_background_job = cur_background_job->next_background_job;
        }
    }
}

job *package_job(background_job *cur_job) {
    job *to_return = malloc(sizeof(job));
    if (to_return == NULL) {
        perror("I am sorry, but there was an error with malloc.\n");
        return NULL;
    }
    to_return->pgid = cur_job->pgid;
    to_return->status = cur_job->status;
    to_return->full_job_string = malloc(lengthOf(cur_job->job_string) + 1);
    if (to_return->full_job_string == NULL) {
        perror("I am sorry, but there was an error with malloc.\n");
        return NULL;
    }
    strcpy(to_return->full_job_string, cur_job->job_string);
    return to_return;
}

void job_suspend_helper(pid_t calling_id) {
    /* if job is in background, just update status */
    background_job *cur_job = all_background_jobs;
    while (cur_job != NULL) {
        if (cur_job->pgid == calling_id) {
            cur_job->status = SUSPENDED;
            cur_job->verbose = TRUE;
            return;
        }
        cur_job = cur_job->next_background_job;
    }

    /* other wise it must be a job being run for a first time */
    job *check_foreground = all_jobs;
    while (check_foreground != NULL) {
        if (check_foreground->pgid == calling_id) {
            check_foreground->suspend_this_job = TRUE;
            return;
        }
        check_foreground = check_foreground->next_job;
    }
}

/* child process has terminated and so we need to remove the process from the linked list (by pid) */
void childReturning(int sig, siginfo_t *siginfo, void *context) {
    int signum = siginfo->si_signo;
    pid_t calling_id = siginfo->si_pid;

    if (signum == SIGCHLD) {
        //in the case of the child being killed, remove it from the list of jobs
        if(siginfo->si_code == CLD_KILLED || siginfo->si_code == CLD_DUMPED || siginfo->si_code == CLD_EXITED) {
            trim_background_process_list(calling_id);
        }
        else if(siginfo->si_code == CLD_STOPPED) {
            job_suspend_helper(calling_id);
        }
    }
}

/* This method is simply the remove node method called when a node needs to be removed from the list of jobs. */
void removeNode(pid_t pidToRemove) {
    //look through jobs for pid of child to remove
    job *currentJob = all_jobs;

    process *currentProcess = NULL;
    process *nextProcess = NULL;

    while (currentJob != NULL) {
        //the pid of the first job process matches, then update job pointer!
        currentProcess = currentJob->first_process;
        if (currentProcess != NULL && currentProcess->pid == pidToRemove) {
            currentJob->first_process = currentProcess->next_process;
            return;
        } else { //look at all processes w/in job for pid not the first one
            while (currentProcess != NULL) {
                nextProcess = currentProcess->next_process;
                if (nextProcess->pid == pidToRemove) {
                    //found the pidToRemove, free, and an reset pointers
                    currentProcess->next_process = nextProcess->next_process;
                    free(nextProcess);
                    return;
                }
                currentProcess = nextProcess;
            }
        }

        //pid not found in list of current processes for prior viewed job, get next job
        currentJob = currentJob->next_job;
    }
}


/* Passes in the command to check. Returns the index of the built-in command if it’s in the array of
 * built-in commands and -1 if it is not in the array allBuiltIns
 */
int isBuiltInCommand(process cmd) {
    for (int i = 0; i < NUMBER_OF_BUILT_IN_FUNCTIONS; i++) {
        if (process_equals(cmd, allBuiltIns[i])) {
           //TODO: comment out when not needed
           if (i == 11) {
               return NOT_FOUND;
           }
            return i; //return index of command
        }
    }
    return NOT_FOUND;
}

int process_equals(process process1, builtin builtin1) {
    int compare_value = strcmp(process1.args[0], builtin1.tag);
    if (compare_value == FALSE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* Passes in the built-in command to be executed along with the index of the command in the allBuiltIns array. This method returns true upon success and false upon failure/error. */
int executeBuiltInCommand(process *process1, int index) {
    //execute built in (testing...)
    return (*(allBuiltIns[index].function))(process1->args);
}

void launchJob(job *j, int foreground) {
    process *p;
    pid_t pid;
    int isBuiltIn;
    for (p = j->first_process; p; p = p->next_process) {
        isBuiltIn = isBuiltInCommand(*p);

        //run as a built-in command
        if (isBuiltIn != NOT_FOUND) {
            executeBuiltInCommand(p, isBuiltIn);
        }
        else {
            /* Fork the child processes.  */
            pid = fork();

            if (pid == 0) {
                /* This is the child process.  */
                launchProcess(p, j->pgid, j->stdin, j->stdout, j->stderr, foreground);
            } else if (pid < 0) {
                /* The fork failed.  */
                perror("fork");
                exit(EXIT_FAILURE);
            } else {
                /* This is the parent process.  */
                p->pid = pid;

                if (!j->pgid) {
                    j->pgid = pid;
                }

                setpgid(pid, j->pgid); //TODO: check process group ids being altered correctly
            }
        }
    }

    if (isBuiltIn == NOT_FOUND) {
        if (foreground) {
            put_job_in_foreground(j, 0);
        } else {
            sigset_t mask;

            if (sigemptyset(&mask) == ERROR) {
                perror("I am sorry, but sigemptyset failed.\n");
                exit(EXIT_FAILURE);
            }

            if (sigaddset(&mask, SIGCHLD) == ERROR) {
                perror("I am sorry, but sigaddset failed.\n");
                exit(EXIT_FAILURE);
            }
            if (sigprocmask(SIG_BLOCK, &mask, NULL) == ERROR) {
                perror("I am sorry, but sigprocmask failed.\n");
                exit(EXIT_FAILURE);
            }

            put_job_in_background(j, !CONTINUE, RUNNING);

            if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == ERROR) {
                perror("I am sorry, but sigprocmask failed.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/* Method to launch our process in either the foreground or the background. */
//method  based off of https://www.gnu.org/software/libc/manual/html_node/Launching-Jobs.html#Launching-Jobs
void launchProcess (process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground) {
    pid_t pid;

    /* Put the process into the process group and give the process group
       the terminal, if appropriate.
       This has to be done both by the shell and in the individual
       child processes because of potential race conditions.  */ //TODO: consider race conditions arising here!!

    pid = getpid();

    if (pgid == 0) {
        pgid = pid;
    }

    if(strcmp(p->args[ZERO], "cat") == 0) {
        foreground = TRUE;
    }

    if (setpgid(pid, pgid) < ZERO) {
        perror("Couldn't put the shell in its own process group.\n");
        exit(EXIT_FAILURE);
    }

    if (foreground) {
        if (tcsetpgrp(shell_terminal, pgid) < 0) {
            perror("tcsetpgrp");
            exit(EXIT_FAILURE);
        }
    }

    /* Set the handling for job control signals back to the default.  */
    if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
        perror("I am sorry, but signal failed.\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGQUIT, SIG_DFL) == SIG_ERR) {
        perror("I am sorry, but signal failed.\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTSTP, SIG_DFL) == SIG_ERR) {
        perror("I am sorry, but signal failed.\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTTIN, SIG_DFL) == SIG_ERR) {
        perror("I am sorry, but signal failed.\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) {
        perror("I am sorry, but signal failed.\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
        perror("I am sorry, but signal failed.\n");
        exit(EXIT_FAILURE);
    }

    if(strcmp(p->args[ZERO], "cat") == 0) {
      cat_builtin(p->args);
    }

    /* Exec the new process.  Make sure we exit.  */
    else if (execvp(p->args[ZERO], p->args) == ERROR) {
        fprintf(stderr, "Error: %s: command not found\n", p->args[0]);
        free_all_jobs();
    }
    exit(EXIT_FAILURE);
}

/* Put job j in the foreground.  If cont is nonzero,
   restore the saved terminal modes and send the process group a
   SIGCONT signal to wake it up before we block.  */

void put_job_in_foreground (job *j, int cont) {
    int status;
    /* Put the job into the foreground.  */
    if (tcsetpgrp(shell_terminal, j->pgid) == ERROR) {
        perror("\"I am sorry, but tcsetpgrp failed.\n");
        exit(EXIT_FAILURE);
    }

    if (tcgetattr(shell_terminal, &j->termios_modes) == ERROR) {
        perror("I am sorry, but tcgetattr failed.\n");
        exit(EXIT_FAILURE);
    }

    /* Wait for it to report.  */
    if(waitpid (j->pgid, &status, WUNTRACED) == ERROR) {
        perror("I am sorry, but waitpid failed.\n");
        exit(EXIT_FAILURE);
    }

    /* Put the shell back in the foreground.  */
    if (tcsetpgrp(shell_terminal, shell_pgid)== ERROR) {
        perror("\"I am sorry, but tcsetpgrp failed.\n");
        exit(EXIT_FAILURE);
    }

    /* Restore the shell’s terminal modes.  */
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}


/* takes background job and gives it to background job */
void simple_background_job_setup(background_job *dest, job *org, int status)
{
    dest->pgid = org->pgid;
    dest->status = status;
    dest->termios_modes = org->termios_modes; // <<< potential source of error here? valgrind and fg seems to be complaing about unitialized bytes
    char *js = malloc(sizeof(char) * lengthOf(org->full_job_string) + 1);
    if(js == NULL) {
        perror("I am sorry, but there was an error with malloc.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(js, org->full_job_string);
    dest->job_string = js;
    dest->verbose = TRUE;
    dest->next_background_job = NULL;
}

/* Put a job in the background initially.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */
void put_job_in_background(job *j, int cont, int status) { //TODO: check on merge here
    /* Add job to the background list with status of running */
    if (!cont) {
        background_job *copyOfJ = malloc(sizeof(background_job));
        if(copyOfJ == NULL) {
            perror("I am sorry, but there was an error with malloc.\n");
            exit(EXIT_FAILURE);
        }
        simple_background_job_setup(copyOfJ, j, status);
        if (all_background_jobs == NULL) {
            all_background_jobs = copyOfJ;
        } else {
            background_job *cur_job = all_background_jobs;
            background_job *next_job = all_background_jobs->next_background_job;
            while (next_job != NULL) {
                background_job *temp = next_job;
                next_job = cur_job->next_background_job;
                cur_job = temp;
            }
            cur_job->next_background_job = copyOfJ;
        }
    }
    else {
        if (kill(-j->pgid, SIGCONT) < 0) {
            perror("kill (SIGCONT)");
        }
    }
}

int arrayLength(char **array) {
    int i = 0;
    while (array[i] != NULL) {
        i++;
    }

    return i;
}

/* Let's have this clean up the job list */
int exit_builtin(char **args) {

    background_job *cur_background_job = all_background_jobs;
    while (cur_background_job != NULL) {
        if (kill(-cur_background_job->pgid, SIGKILL) < 0){
            perror("kill (SIGKILL)");
        }
        else {
            printf("KILLED pgid: %d job: %s \n", cur_background_job->pgid, cur_background_job->job_string);
        }
        cur_background_job = cur_background_job->next_background_job;
    }

    EXIT = TRUE;
    return EXIT; //success
}

void background_built_in_helper(background_job *bj, int cont, int status) {
    if (kill(-bj->pgid, SIGCONT) < 0) {
        perror("kill (SIGCONT)");
    }

    background_job *current_job = all_background_jobs;
    int index = 0;
    while (current_job != NULL) {
        index ++;
        if (current_job->pgid == bj->pgid) {
            current_job->status = RUNNING;
            printf("[%d]\t\t%s\n", index, bj->job_string);
        }
        current_job = current_job->next_background_job;
    }
}

background_job *get_background_from_pgid(pid_t pgid) {
    background_job *current_job = all_background_jobs;
    while (current_job != NULL) {
        if (current_job->pgid == pgid) {
            return current_job;
        }
        current_job = current_job->next_background_job;
    }
    return NULL;
}

void foreground_helper(background_job *bj) {
    int status;
    /* Put the job into the foreground.  */
    if (tcsetpgrp(shell_terminal, bj->pgid) == ERROR) {
        perror("\"I am sorry, but tcsetpgrp failed.\n");
    }

    if (tcgetattr(shell_terminal, &bj->termios_modes) == ERROR) {
        perror("I am sorry, but tcgetattr failed.\n");
    }

    /* Send the job a continue signal, if necessary.  */
    if (tcsetattr(shell_terminal, TCSADRAIN, &bj->termios_modes) == ERROR) {
        perror("I am sorry, but tcsetattr failed.\n");
    }

    if (kill(-bj->pgid, SIGCONT) < 0)
        perror("kill (SIGCONT)");

    printf("%s\n", bj->job_string); //print statement

    /* if the system call is interrupted, wait again */
    waitpid(bj->pgid , &status, WUNTRACED);

    /* Put the shell back in the foreground.  */
    if (tcsetpgrp(shell_terminal, shell_pgid) == ERROR) {
        perror("\"I am sorry, but tcsetpgrp failed.\n");
    }

    /* Restore the shell’s terminal modes.  */
    if (tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes) == ERROR) {
        perror("tcsetattr");
    }
}

/* Method to take a job id and send a SIGTERM to terminate the process.*/
int kill_builtin(char **args) {
    char *flag = "-9\0";
    int flagLocation = 1;
    int pidLocationNoFlag = 1;
    int pidLocation = 2;
    int minElements = 2;
    int maxElements = 3;

    //get args length
    int argsLength = arrayLength(args);

    if (argsLength < minElements || argsLength > maxElements) {
        //invalid arguments
        fprintf(stderr,"I am sorry, but you have passed an invalid number of arguments to kill.\n");
        return FALSE;
    } else if (argsLength == maxElements && args[pidLocation][ZERO] == '%') {
        if (strcmp(args[flagLocation], flag) ==
            ZERO) { //check that -9 flag was input correctly, otherwise try sending kill with pid
            //(error checking gotten from stack overflow)
            const char *nptr = args[pidLocation] + pidLocationNoFlag;  /* string to read as a number      */
            char *endptr = NULL;                            /* pointer to additional chars  */
            int base = 10;                                  /* numeric base (default 10)    */
            long long int number = 0;                       /* variable holding return      */

            /* reset errno to 0 before call */
            errno = 0;

            /* call to strtol assigning return to number */
            number = strtoll(nptr, &endptr, base);

            /* test return to number and errno values */
            if (nptr == endptr) {
                printf(" number : %lld  invalid  (no digits found, 0 returned)\n", number);
                return FALSE;
            } else if (errno == ERANGE && number == LONG_MIN) {
                printf(" number : %lld  invalid  (underflow occurred)\n", number);
                return FALSE;
            } else if (errno == ERANGE && number == LONG_MAX) {
                printf(" number : %lld  invalid  (overflow occurred)\n", number);
                return FALSE;
            } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
                printf(" number : %lld  invalid  (base contains unsupported value)\n", number);
                return FALSE;
            } else if (errno != ZERO && number == ZERO) {
                printf(" number : %lld  invalid  (unspecified error occurred)\n", number);
                return FALSE;
            } else if (errno == ZERO && nptr && *endptr != ZERO) {
                printf(" number : %lld    invalid  (since additional characters remain)\n", number);
                return FALSE;
            }

            //have location now in linked list
            int currentNode = 0;
            background_job *currentJob = all_background_jobs;

            while (currentJob != NULL) {
                currentNode++;
                //found your node
                if (currentNode == number) {
                    break;
                } else {
                    currentJob = currentJob->next_background_job;
                }
            }

            //node was not found!
            if (currentNode < number || number <= ZERO) {
                fprintf(stderr,"I am sorry, but that job does not exist.\n");
                return FALSE;
            } else {
                pid_t pid = currentJob->pgid;
                printf("Sent SIGKILL to %d, check jobs to see completed\n", pid);
                if (kill(pid, SIGKILL) == ERROR) {
                    perror("I am sorry, an error occurred with kill.\n");
                    return FALSE; //error occurred
                }
            }
        }
    } else { //we have no flags and only kill with a pid
        if (args[pidLocationNoFlag][ZERO] == '%') {
            //PID is second argument
            //(error checking gotten from stack overflow)
            const char *nptr =
                    args[pidLocationNoFlag] + pidLocationNoFlag;  /* string to read as a number      */
            char *endptr = NULL;                            /* pointer to additional chars  */
            int base = 10;                                  /* numeric base (default 10)    */
            long long int number = 0;                       /* variable holding return      */

            /* reset errno to 0 before call */
            errno = 0;

            /* call to strtol assigning return to number */
            number = strtoll(nptr, &endptr, base);

            /* test return to number and errno values */
            if (nptr == endptr) {
                fprintf(stderr, " number : %lld  invalid  (no digits found, 0 returned)\n", number);
                return FALSE;
            } else if (errno == ERANGE && number == LONG_MIN) {
                fprintf(stderr, " number : %lld  invalid  (underflow occurred)\n", number);
                return FALSE;
            } else if (errno == ERANGE && number == LONG_MAX) {
                fprintf(stderr, " number : %lld  invalid  (overflow occurred)\n", number);
                return FALSE;
            } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
                fprintf(stderr, " number : %lld  invalid  (base contains unsupported value)\n", number);
                return FALSE;
            } else if (errno != 0 && number == 0) {
                fprintf(stderr, " number : %lld  invalid  (unspecified error occurred)\n", number);
                return FALSE;
            } else if (errno == 0 && nptr && *endptr != 0) {
                fprintf(stderr, " number : %lld    invalid  (since additional characters remain)\n", number);
                return FALSE;
            }

            //have location now in linked list
            int currentNode = 0;
            background_job *currentJob = all_background_jobs;

            while (currentJob != NULL) {
                currentNode++;
                //found your node
                if (currentNode == number) {
                    break;
                } else {
                    currentJob = currentJob->next_background_job;
                }
            }

            //node was not found!
            if (currentNode < number || number <= 0) {
                fprintf(stderr,"I am sorry, but that job does not exist.\n");
                return FALSE;
            } else {
                pid_t pid = currentJob->pgid;
                printf("Sent SIGTERM to %d, check jobs to see completed\n", pid);
                if (kill(pid, SIGTERM) == -1) {
                    fprintf(stderr,"I am sorry, an error occurred with kill.\n");
                    return FALSE; //error occurred
                }
            }
        }
        return FALSE;
    }
    return FALSE;
}

/* Method to iterate through the linked list and print out node parameters. */
int jobs_builtin(char **args) {
    background_job *currentJob = all_background_jobs;
    char *status[] = {running, suspended, killed};
    int jobID = 1;

    if (currentJob == NULL) {} // do nothing
    else {
        while (currentJob != NULL) {
            //print out formatted information for processes in job
            printf("[%d]\t %d %s \t\t %s\n", jobID, currentJob->pgid, status[currentJob->status],
                   currentJob->job_string);
            jobID++;

            //get next job
            currentJob = currentJob->next_background_job;
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

/* Method that sends continue signal to suspended process in background -- this is bg*/
int background_builtin(char **args) {
    //get size of args
    int argsLength = arrayLength(args);
    int locationOfPercent = 1;
    int minArgsLength = 1;
    int maxArgsLength = 2;

    if (argsLength < minArgsLength || argsLength > maxArgsLength) {
        fprintf(stderr, "I am sorry, but that is an invalid list of commands to bg.\n");
        return FALSE;
    }

    if (argsLength == minArgsLength) {
        //bring back tail of jobs list, if it exists
        background_job *currentJob = all_background_jobs;
        background_job *nextJob = NULL;
        if (currentJob == NULL) {
            fprintf(stderr,"I am sorry, but that job does not exist.\n");
            return FALSE;
        }

        while (currentJob != NULL) {
            nextJob = currentJob->next_background_job;
            if (nextJob == NULL) {
                break; //want to bring back current job
            }
            currentJob = currentJob->next_background_job;
        }

        sigset_t mask;

        if (sigemptyset(&mask) == ERROR) {
            perror("I am sorry, but sigemptyset failed.\n");
            exit(EXIT_FAILURE);
        }

        if (sigaddset(&mask, SIGCHLD) == ERROR) {
            perror("I am sorry, but sigaddset failed.\n");
            exit(EXIT_FAILURE);
        }
        if (sigprocmask(SIG_BLOCK, &mask, NULL) == ERROR) {
            perror("I am sorry, but sigprocmask failed.\n");
            exit(EXIT_FAILURE);
        }

        background_built_in_helper(currentJob, TRUE, RUNNING);

        if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == ERROR) {
            perror("I am sorry, but sigprocmask failed.\n");
            exit(EXIT_FAILURE);
        }

        return TRUE; //success!
    } else if (argsLength == maxArgsLength && args[locationOfPercent][0] == '%') {
        //(error checking gotten from stack overflow)
        const char *nptr = args[locationOfPercent] + locationOfPercent;     /* string to read as a number   */
        char *endptr = NULL;                            /* pointer to additional chars  */
        int base = 10;                                  /* numeric base (default 10)    */
        long long int number = 0;                       /* variable holding return      */

        printf("Number to parse: %s\n", nptr);
        /* reset errno to 0 before call */
        errno = 0;

        /* call to strtol assigning return to number */
        number = strtoll(nptr, &endptr, base);

        /* test return to number and errno values */
        if (nptr == endptr) {
            fprintf(stderr, " number : %lld  invalid  (no digits found, 0 returned)\n", number);
            return FALSE;
        } else if (errno == ERANGE && number == LONG_MIN) {
            fprintf(stderr, " number : %lld  invalid  (underflow occurred)\n", number);
            return FALSE;
        } else if (errno == ERANGE && number == LONG_MAX) {
            fprintf(stderr, " number : %lld  invalid  (overflow occurred)\n", number);
            return FALSE;
        } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
            fprintf(stderr, " number : %lld  invalid  (base contains unsupported value)\n", number);
            return FALSE;
        } else if (errno != ZERO && number == ZERO) {
            fprintf(stderr, " number : %lld  invalid  (unspecified error occurred)\n", number);
            return FALSE;
        } else if (errno == ZERO && nptr && *endptr != ZERO) {
            fprintf(stderr, " number : %lld    invalid  (since additional characters remain)\n", number);
            return FALSE;
        }

        //have location now in linked list
        int currentNode = 0;
        background_job *currentJob = all_background_jobs;

        while (currentJob != NULL) {
            currentNode++;
            //found your node
            if (currentNode == number) {
                /* sig proc mask this */
                sigset_t mask;
                if (sigemptyset(&mask) == ERROR) {
                    perror("I am sorry, but sigemptyset failed.\n");
                    exit(EXIT_FAILURE);
                }

                if (sigaddset(&mask, SIGCHLD) == ERROR) {
                    perror("I am sorry, but sigaddset failed.\n");
                    exit(EXIT_FAILURE);
                }
                if (sigprocmask(SIG_BLOCK, &mask, NULL) == ERROR) {
                    perror("I am sorry, but sigprocmask failed.\n");
                    exit(EXIT_FAILURE);
                }

                background_built_in_helper(currentJob, TRUE, RUNNING);

                if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == ERROR) {
                    perror("I am sorry, but sigprocmask failed.\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                currentJob = currentJob->next_background_job;
            }
        }

        //node was not found!
        if (currentNode < number || number <= 0) {
            fprintf(stderr,"I am sorry, but that job does not exist.\n");
        }
    }

    return FALSE;
}

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground_builtin(char** args) {
    /*
     * Basic functionality included (ie foreground last pid)
     * to test to make sure this is working
     */

    int percentLocation = 1;
    int minElements = 1;
    int maxElements = 2;

    //get args length
    int argsLength = arrayLength(args);

    if (argsLength < minElements || argsLength > maxElements) {
        //invalid arguments
        fprintf(stderr, "I am sorry, but you have passed an invalid number of arguments to fg.\n");
        return FALSE;
    } else if (argsLength == maxElements) {
        if (args[percentLocation][0] == '%') {
            //(error checking gotten from stack overflow)
            const char *nptr =
                    args[percentLocation] + percentLocation;  /* string to read as a number      */
            char *endptr = NULL;                            /* pointer to additional chars  */
            int base = 10;                                  /* numeric base (default 10)    */
            long long int number = 0;                       /* variable holding return      */

            /* reset errno to 0 before call */
            errno = 0;

            printf("job number %s\n", nptr);

            /* call to strtol assigning return to number */
            number = strtoll(nptr, &endptr, base);

            /* test return to number and errno values */
            if (nptr == endptr) {
                fprintf(stderr," number : %lld  invalid  (no digits found, 0 returned)\n", number);
                return FALSE;
            } else if (errno == ERANGE && number == LONG_MIN) {
                fprintf(stderr," number : %lld  invalid  (underflow occurred)\n", number);
                return FALSE;
            } else if (errno == ERANGE && number == LONG_MAX) {
                fprintf(stderr," number : %lld  invalid  (overflow occurred)\n", number);
                return FALSE;
            } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
                fprintf(stderr," number : %lld  invalid  (base contains unsupported value)\n", number);
                return FALSE;
            } else if (errno != 0 && number == 0) {
                fprintf(stderr," number : %lld  invalid  (unspecified error occurred)\n", number);
                return FALSE;
            } else if (errno == 0 && nptr && *endptr != 0) {
                fprintf(stderr," number : %lld    invalid  (since additional characters remain)\n", number);
                return FALSE;
            }

            //have location now in linked list
            int currentNode = 0;
            background_job *currentJob = all_background_jobs;

            while (currentJob != NULL) {
                currentNode++;
                //found your node requested
                if (currentNode == number) {

                    foreground_helper(currentJob);

                    return TRUE;
                } else {
                    currentJob = currentJob->next_background_job;
                }
            }

            //node was not found!
            if (currentNode < number || number <= 0) {
                fprintf(stderr,"I am sorry, but that job does not exist.\n");
                return FALSE;
            }
        } else {
            fprintf(stderr,"I am sorry, but you have passed an invalid node argument to fg.\n");
            return FALSE;
        }
    } else {

        //bring back tail of jobs list, if it exists
        background_job *currentJob = all_background_jobs;
        background_job *nextJob = NULL;
        if (currentJob == NULL) {
            fprintf(stderr,"I am sorry, but that job does not exist.\n");
            return FALSE;
        }

        //go to the end of the list
        while (currentJob != NULL) {
            nextJob = currentJob->next_background_job;
            if (nextJob == NULL) {
                break; //want to bring back current job
            }
            currentJob = currentJob->next_background_job;
        }

        foreground_helper(currentJob);

        return TRUE;
    }
    return FALSE;
}

void print_args(char **args) {
    //get args length
    int argsLength = arrayLength(args);
    for (int i = 0; i < argsLength; i++) {
        printf("%s\n", args[i]);
    }
}

//TODO: fill in the following methods :)
int ls_builtin(char **args) {
//    print_args(args);
    int args_length = arrayLength(args);

    if (args_length == 1) {
        int file_table_index = get_fd_from_inode_value(pwd_directory->inode_index);
        printf("file_table_index: %d\n", file_table_index);
        f_rewind(file_table_index);
        inode* node = get_table_entry(file_table_index)->file_inode;
        printf("node size: %d\n",node->size );
        directory_entry *entry = f_readdir(file_table_index);
        entry = f_readdir(file_table_index);
        entry = f_readdir(file_table_index);
        while (entry != NULL) {
            printf("%s\n", entry->filename);
            free(entry);
            entry = f_readdir(file_table_index);
        }
        return TRUE;
    } else if (args_length == 2) { //case where there are flags :)
        if (strcmp(F, args[1]) == 0 || strcmp(l, args[1]) == 0) {
            int file_table_index = get_fd_from_inode_value(pwd_directory->inode_index);
            f_rewind(file_table_index);
            if (strcmp(F, args[1]) == 0) {
                directory_entry *entry = f_readdir(file_table_index);
                entry = f_readdir(file_table_index);
                entry = f_readdir(file_table_index);
                while (entry != NULL) {
                    inode *inode1 = get_inode(entry->inode_index);
                    if (inode1->type == DIR) {
                        printf("%s/\n", entry->filename);
                    } else {
                        printf("%s\n", entry->filename);
                    }
                    entry = f_readdir(file_table_index);
                    free(inode1);
                }
                return TRUE;
            } else {
                //-l flag
                //TODO: fill this in here...

            }
        } else {
            perror("An error occurred in ls. Please try again.\n");
            return FALSE;
        }
    } else {
        perror("An error occurred in ls. Please try again.\n");
        return FALSE;
    }

    return FALSE;
}

int chmod_builtin(char **args) {

    return 0;
}

int mkdir_builtin(char **args) {
    if (args[1] == NULL) {
        printf("%s\n", "incorrect format for mkdir");
        return EXITFAILURE;
    }
    char *newfolder = NULL;
    char *path = malloc(strlen(args[1]));
    memset(path, 0, strlen(args[1]));
    char path_copy[strlen(args[1]) + 1];
    char copy[strlen(args[1]) + 1];
    strcpy(path_copy, args[1]);
    strcpy(copy, args[1]);
    char *s = "/";
    //calculate the level of depth of dir
    char *token = strtok(copy, s);
    int count = 0;
    while (token != NULL) {
        count++;
        token = strtok(NULL, s);
    }
    printf("count: %d\n", count);
    newfolder = strtok(path_copy, s);
    while (count > 1) {
        count--;
        printf("new_folder: %s\n", newfolder);
        path = strcat(path, newfolder);
        path = strcat(path, "/");
        newfolder = strtok(NULL, s);
    }
    printf("path: %s\n", path);
    char *absolute_path = convert_absolute(path);
    printf("converted: %s\n", absolute_path);
    free(path);
    printf("newfolder: %s\n", newfolder);
    char *result = malloc(strlen(absolute_path) + 1 + strlen(newfolder) + 1);
    memset(result, 0, strlen(absolute_path) + 1 + strlen(newfolder) + 1);
    result = strncat(result, absolute_path, strlen(absolute_path));
    result = strncat(result, "/", 1);
    result = strcat(result, newfolder);
    free(absolute_path);
    printf("resuting string: %s\n", result);
    directory_entry *entry = f_mkdir(result);
    free(entry);
    free(result);
    return 0;
}

int rmdir_builtin(char **args) {
    char* filename = args[1];
    print_file_table();
    char* filepath = convert_absolute(pwd_directory->filename);
    char* wholepath = malloc(strlen(filename)+strlen(filepath)+2);
    memset(wholepath, 0, strlen(filename)+strlen(filepath)+2);
    wholepath = strncat(wholepath, filepath, strlen(filepath));
    wholepath = strncat(wholepath, "/", 1);
    wholepath = strcat(wholepath,filename);
    printf("wholepath: %s\n", wholepath);
    f_remove(wholepath);
    return 0;
}

int cd_builtin(char **args) {
    if (arrayLength(args) == 1) {
        //go to the root directory
        pwd_directory = goto_root();
    } else {
        pwd_directory = goto_destination(args[1]);
    }
    return 0;
}

int pwd_builtin(char **args) {
    char *absolute_path = convert_absolute(pwd_directory->filename);
    printf("%s\n", absolute_path);
    free(absolute_path);
    return 0;
}

// directory_entry *in_valid_path(char *file_name) {
//     directory_entry *directory_to_search = f_opendir(convert_absolute(file_name));
//     int file_table_index = get_fd_from_inode_value(pwd_directory->inode_index);
//     f_rewind(file_table_index);
//     directory_entry *entry = f_readdir(file_table_index);
//     while (entry != NULL) {
//         if (strcmp(entry->filename, file_name) == 0) {
//             found = entry;
//             break;
//         } else {
//             entry = f_readdir(file_table_index);
//         }
//     }
//
//     return found;
// }

char *which_is_contained(char *token) {
    for (int i = 0; i < strlen(token); i++) {
        char c = token[i];
        if (c == '<') {
            return "<";
        } else if (c == '>') {
            if (i + 1 < strlen(token)) {
                    return ">>";
                } else {
                    return ">";
                }
            } else {
                return ">";
            }
        }

    return NULL;
}

int contains_delimiter(char **args, int start, int end) {
  for(int i=start; i<end; i++) {
    if((strcmp(args[i], ">") ==0) || (strcmp(args[i], ">>") ==0) || (strcmp(args[i], "<") ==0)) {
      return i;
    }
  }

  return -1;
}

int location_last_delimiter(char **args) {
  int length_args = arrayLength(args);
  char *delim;
  boolean last_found = FALSE;
  int return_value;
  int last_found_location = -1;
  while(!last_found) {
    for(int i=0; i<length_args-1; i++) {
      if((return_value = contains_delimiter(args, i, length_args)) != NULL) {
        last_found_location = return_value;
      } else {
        last_found = TRUE;
        break;
      }
    }
  }

  return last_found_location;
}

// boolean all_strings(char **args, int start, int stop) {
//   for(int i=start; i<=stop; i++) {
//     if(in_directory(args[i]) != NULL) {
//       return FALSE;
//     } else {
//       return TRUE;
//     }
//   }
// }

boolean is_all_delimeters(char **args) {
  int args_length = arrayLength(args);

  for(int i=1; i<args_length; i++) {
    if(!((strcmp(args[i], ">") ==0) || (strcmp(args[i], ">>") ==0) || (strcmp(args[i], "<") ==0))) {
      return FALSE;
    }
  }

  return TRUE;
}

int cat_builtin(char **args) {
    //get args length
    int args_length = arrayLength(args);

    printf("Printing cat args result:\n");
    print_args(args);

   if (args_length == 1) {
       char c;
       while (read(STDIN_FILENO, &c, 1) > 0) {
           if (c != '\n') {
               if (write(1, &c, 1) < 0) {
                   errorMessage();
               }
           } else {
               if (write(STDOUT_FILENO, "\n", 1) < 0) {
                   errorMessage();
               }
           }
       }
   } else {
     printf("args len %d\n", args_length);
     int location = -1;
     int last_found_location = -1;
     char *delim = NULL;
     int flag = READ;

     printf("got here \n");
     location = contains_delimiter(args, 0, args_length);
     printf("location %d \n", location);

     if(location != -1) {
       if(is_all_delimeters(args)) {
         printf("syntax error in cat\n");
         return -1;
       }

        delim = which_is_contained(args[location]);
        // last_found_location = location_last_delimiter(args);
        printf("got here\n");
        if(location == 1) {
          printf("got here\n");
          // if(args[last_found_location] == ">>") {
          if(strcmp(args[location], ">>") == 0) {
            flag = APPEND;
          }
          // } else if(args[last_found_location] == ">") {
          else if(strcmp(args[location], ">") == 0) {
            flag = WRITE;
          }

          // int i = last_found_location + 1; //TODO: fix once last known is current_working_dir
          int i = location + 1;

          char *newfolder = NULL;
          char *path = malloc(strlen(args[i]));
          memset(path, 0, strlen(args[i]));
          char path_copy[strlen(args[i]) + 1];
          char copy[strlen(args[i]) + 1];
          strcpy(path_copy, args[i]);
          strcpy(copy, args[i]);
          char *s = "/";
          //calculate the level of depth of dir
          char *token = strtok(copy, s);
          int count = 0;
          while (token != NULL) {
              count++;
              token = strtok(NULL, s);
          }
          printf("count: %d\n", count);
          newfolder = strtok(path_copy, s);
          while (count > 1) {
              count--;
              printf("new_folder: %s\n", newfolder);
              path = strcat(path, newfolder);
              path = strcat(path, "/");
              newfolder = strtok(NULL, s);
          }
          printf("path: %s\n", path);
          char *absolute_path = convert_absolute(path);
          printf("converted: %s\n", absolute_path);
          free(path);
          printf("newfolder: %s\n", newfolder);
          char *result = malloc(strlen(absolute_path) + 1 + strlen(newfolder) + 1);
          memset(result, 0, strlen(absolute_path) + 1 + strlen(newfolder) + 1);
          result = strncat(result, absolute_path, strlen(absolute_path));
          result = strncat(result, "/", 1);
          result = strcat(result, newfolder);
          free(absolute_path);
          printf("resuting string for file path: %s\n", result);

        int fd = f_open(result, flag, NULL);

        printf("opened new file in cat...\n");
        printf("file descriptor %d\n", fd);
        // while (read(STDIN_FILENO, c, 1) > 0) {
        //   if (f_write(c, 1, 1, fd) < 0) {
        //       errorMessage();
        //   }
        // }

        char c;
        while (read(STDIN_FILENO, &c, 1) > 0) {
            // if (c != '\n') {
              if (f_write(&c, 1, 1, fd) < 0) {
                    errorMessage();
                }
          }

        f_close(fd);

            // } else {
            //
            //     if (write(STDOUT_FILENO, "\n", 1) < 0) {
            //         errorMessage();
            //     }
            // }
        // }

      }

      // while (read(STDIN_FILENO, &c, 1) > 0) {
      //     if (c != '\n') {
      //         if (write(1, &c, 1) < 0) {
      //             errorMessage();
      //         }
      //     } else {
      //         if (write(STDOUT_FILENO, "\n", 1) < 0) {
      //             errorMessage();
      //         }
      //     }
      // }

          /*   char c;
            while (read(STDIN_FILENO, &c, 1) > 0) {
                  (write(1, &c, 1) < 0) {
                      errorMessage();
                  }
                }
                */
        // }
        //append
        // if(delim == ">>") {
        //   // if(all_strings(args, 1, location-1) == TRUE) {
        //
        // //   } else {
        // //     //TODO: finish this
        // //   }
        // // } else if(delim = ">") { //overwrite
        // //
        // // } else { //accept input with "<"
        // //
        // }
     } else {
       printf("got here\n");
         inode *inode1;
         directory_entry *entry;
         for (int i = 1; i < args_length; i++) {
             //first open the directory sought and then check in that directory for the value
             char *newfolder = NULL;
             char *path = malloc(strlen(args[i]));
             memset(path, 0, strlen(args[i]));
             char path_copy[strlen(args[i]) + 1];
             char copy[strlen(args[i]) + 1];
             strcpy(path_copy, args[i]);
             strcpy(copy, args[i]);
             char *s = "/";
             //calculate the level of depth of dir
             char *token = strtok(copy, s);
             int count = 0;
             while (token != NULL) {
                 count++;
                 token = strtok(NULL, s);
             }
             printf("count: %d\n", count);
             newfolder = strtok(path_copy, s);
             while (count > 1) {
                 count--;
                 printf("new_folder: %s\n", newfolder);
                 path = strcat(path, newfolder);
                 path = strcat(path, "/");
                 newfolder = strtok(NULL, s);
             }
             printf("path: %s\n", path);
             char *absolute_path = convert_absolute(path);
             printf("converted: %s\n", absolute_path);
             free(path);
             printf("newfolder: %s\n", newfolder);
             char *result = malloc(strlen(absolute_path) + 1 + strlen(newfolder) + 1);
             memset(result, 0, strlen(absolute_path) + 1 + strlen(newfolder) + 1);
             result = strncat(result, absolute_path, strlen(absolute_path));
             result = strncat(result, "/", 1);
             result = strcat(result, newfolder);
             free(absolute_path);
             printf("resuting string: %s\n", result);

             int fd;
             if ((fd = f_open(result, READ, NULL)) != EXITFAILURE) {
                 //print the file to the screen
                 inode1 = get_table_entry(fd)->file_inode;
                 if (inode1->type == DIR) {
                     printf("cat: %s: Is a directory\n", args[i]);
                 } else {
                     int file_size = inode1->size;
                     void *file = malloc(sizeof(file_size));
                     if(file == NULL) {
                       perror("Malloc\n");
                       return -1;
                     }

                     f_read(file, file_size, 1, fd);
                     if (write(STDOUT_FILENO, file, file_size) < 0) {
                         errorMessage();
                     }
                     printf("\n");
                     free(file);
                     f_close(fd);
                 }
             } else {
                 printf("cat: %s: Error opening file or file does not exist\n", args[i]);
             }
         }
     }
 }

    return 0;
}


/*
 * Error method to handle any error in program
 */
void errorMessage() {
    int lengthOfString = 26;
    write(1, "Unforeseen error occurred.\n\0", lengthOfString);
    exit(EXIT_FAILURE);
}

int more_builtin(char **args) {
    return 0;
}

int rm_builtin(char **args) {
    rmdir_builtin(args);

    return 0;
}

int mount_builtin(char **args) {
    return 0;
}

int unmount_builtin(char **args) {
    return 0;
}

directory_entry* goto_root() {
    // inode *root = get_table_entry(0)->file_inode;
    // print_inode(root);
    // int cur_inode_index = pwd_directory->inode_index;
    // int cur_fd = get_fd_from_inode_value(cur_inode_index);
    // inode *curnode = get_table_entry(cur_fd)->file_inode;
    // remove_from_file_table(curnode);
    // addto_file_table(root, APPEND);
    // pwd_directory->inode_index = 0;
    // // strcpy(pwd_directory->filename, "/");
    // pwd_directory->filename[0] = '/';
    // pwd_directory->filename[1] = 0;
    // print_file_table();
    goto_destination("/");
    return pwd_directory;
}

directory_entry* goto_destination(char* filepath) {
    char copy[strlen(filepath) + 1];
    strcpy(copy, filepath);
    char *s = "/";
    char *token = strtok(copy, s);
    directory_entry *current_working_dir = malloc(sizeof(directory_entry));
    current_working_dir->inode_index = pwd_directory->inode_index;
    strcpy(current_working_dir->filename, pwd_directory->filename);
    inode *curnode = NULL;
    if (filepath[0] != '/') {
        printf("%s\n", "relative path");
        //relative path
        for (; token != NULL; token = strtok(NULL, s)) {
            if (strcmp(token, ".") != 0) {
                printf("token :%s\n", token);
                printf("cur inode index: %d\n", current_working_dir->inode_index);
                int cur_fd = get_fd_from_inode_value(current_working_dir->inode_index);
                printf("cur_fd: %d\n", cur_fd);
                curnode = get_table_entry(cur_fd)->file_inode;
                inode *prev_node = curnode;
                //print_inode(curnode);
                int current_fd = get_fd_from_inode_value(curnode->inode_index);
                f_rewind(current_fd);
                printf("current_fd: %d\n", current_fd);
                directory_entry *entry = NULL;
                int size = curnode->size;
                for (int i = 0; i < size; i += sizeof(directory_entry)) {
                    entry = f_readdir(current_fd);
                    if (entry == NULL) {
                        printf("%s\n", "Not found");
                        free(entry);
                        free(current_working_dir);
                        return NULL;
                    }
                    printf("entry_name: %s\n", entry->filename);
                    if (strcmp(entry->filename, token) == 0) {
                        if (prev_node != NULL && prev_node->inode_index != 0) {
                            printf("curnode index: %d\n", curnode->inode_index);
                            remove_from_file_table(curnode);
                        }
                        printf("found: %s\n", token);
                        current_working_dir->inode_index = entry->inode_index;
                        char* name = get_parentdir_name(entry);
                        printf("tesing: %s\n", name);
                        strcpy(current_working_dir->filename, name);
                        // strcpy(current_working_dir->filename, entry->filename);
                        curnode = get_inode(current_working_dir->inode_index);
                        inode *parent_node = get_inode(curnode->parent_inode_index);
                        int parent_fd = addto_file_table(parent_node, APPEND);
                        addto_file_table(curnode, APPEND);
                        free(entry);
                        break;
                    }
                    free(entry);
                }
                size = curnode->size;
                f_rewind(current_fd);
            }
        }
    } else {
        printf("%s\n", "absolute path");
        //absolute path
        directory_entry *entry = f_opendir(filepath);
        if (entry == NULL) {
            printf("cannot go to :%s \n", filepath);
            free(entry);
            free(current_working_dir);
            return NULL;
        } else {
            current_working_dir->inode_index = entry->inode_index;
            strcpy(current_working_dir->filename,entry->filename);
            curnode = get_inode(current_working_dir->inode_index);
            inode* parent_node = get_inode(curnode->parent_inode_index);
            int parent_fd = addto_file_table(parent_node, APPEND);
            addto_file_table(curnode, APPEND);
            free(entry);
        }
    }
    // printf("distination exists: %s\n", filepath );
    // free(pwd_directory);
    pwd_directory = current_working_dir;
    // print_file_table();
    printf("pwd_dir: %s\n", pwd_directory->filename);
    return pwd_directory;
}

char* get_parentdir_name(directory_entry* entry){
  printf("%s\n","in get_parentdir_name" );
  if(strcmp(entry->filename, "..") == 0){
    inode* parent = get_inode(entry->inode_index);
    inode* grand_parent = get_inode(parent->parent_inode_index);
    int size = grand_parent->size;
    int grand_fd = get_fd_from_inode_value(grand_parent->inode_index);
    f_rewind(grand_fd);
    if(grand_fd == -1){
      printf("%s\n", "grand_fd not in the file table");
    }
    for(int i=0; i<size; i+=sizeof(directory_entry)){
      directory_entry* ent = f_readdir(grand_fd);
      if(ent->inode_index == parent->inode_index){
        free(parent);
        free(grand_parent);
        return ent->filename;
      }
      if(ent == NULL){
        printf("%s\n", "something wrong in get_parentdir_name");
      }
      free(ent);
    }
  }else{
    return entry->filename;
  }

}

/*only take in a dir path*/
char* convert_absolute(char* filepath){
  directory_entry* dest = NULL;
  if((dest = goto_destination(filepath)) == NULL){
    printf("%s does not exists\n", filepath);
    return NULL;
  }
  directory_entry* destination = malloc(sizeof(directory_entry));
  destination->inode_index = dest->inode_index;
  strcpy(destination->filename,dest->filename);
  printf("in convert_absolute\n");
  char* absolute_path_collection[FILENAMEMAX];
  directory_entry* cur = destination;
  printf("destination_filename: %s\n", destination->filename);
  printf("destination_inode: %d\n", destination->inode_index);
  // print_file_table();
  int cur_fd = get_fd_from_inode_value(cur->inode_index);
  printf("cur_fd: %d\n", cur_fd);
  inode* cur_node = get_table_entry(cur_fd)->file_inode;
  int parent_fd = get_fd_from_inode_value(cur_node->parent_inode_index);
  inode* parent_node = get_table_entry(parent_fd)->file_inode;
  int old_parent_index = parent_node->inode_index;
  int count = 0;
  int size = parent_node->size;
  f_rewind(parent_fd);
  for(; cur->inode_index > 0;){
    for(int i = 0; i < size; i += sizeof(directory_entry)){
      printf("parent_fd: %d\n", parent_fd);
      directory_entry* entry = f_readdir(parent_fd);
      if(entry == NULL){
        printf("something wrong in conver_absolute\n");
        printf("%s not found\n", filepath);
        free(destination);
        return NULL;
      }
      printf("entry->filename: %s\n", entry->filename);
      printf("cur->inode_index: %d\n", cur->inode_index);
      printf("entry->inode_index: %d\n", entry->inode_index);
      if(strcmp(entry->filename, "..")==0){
        printf("%s\n", "find parent in conver_absolute");
        parent_node= get_inode(entry->inode_index);
        // addto_file_table(parent_node, APPEND);
      }
      if(entry->inode_index == cur->inode_index){
        printf("%s\n", "FOUND" );
        parent_fd = get_fd_from_inode_value(parent_node->inode_index);
        // remove_from_file_table(cur_node);
        cur_fd = get_fd_from_inode_value(old_parent_index);
        printf("cur_fd: %d\n", cur_fd);
        cur_node = get_table_entry(cur_fd)->file_inode;
        printf("%s\n", entry->filename);
        absolute_path_collection[count] = malloc(strlen(entry->filename)+1);
        strcpy(absolute_path_collection[count], entry->filename);
        break;
      }
      free(entry);
    }
    size = parent_node->size;
    printf("%s\n", "is it here?");
    cur->inode_index =  old_parent_index;
    printf("reset to :%d\n", old_parent_index);
    old_parent_index = parent_node->inode_index;
    count ++;
    f_rewind(parent_fd);
    free(parent_node);
  }
  free(destination);
  char* absolute_path = NULL;
  if(count == 0){
    absolute_path = malloc(2);
    memset(absolute_path, 0, 2);
    absolute_path = strcat(absolute_path, "/");
    // print_file_table();
    printf("%s\n", "count == 0");
    return absolute_path;
  }
  absolute_path = malloc(count*FILENAMEMAX );
  memset(absolute_path, 0 , count*FILENAMEMAX);
  while(count > 0){
    count --;
    absolute_path = strcat(absolute_path, "/");
    absolute_path = strcat(absolute_path, absolute_path_collection[count]);
    free(absolute_path_collection[count]);
  }
  // print_file_table();
  printf("pwd: %s\n", pwd_directory->filename);
  return absolute_path;
}
