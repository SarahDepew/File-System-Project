//
// Created by Sarah Depew on 4/27/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <memory.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "boolean.h"
#include "filesystem.h"

#define MINNUMARGUMENTS 2
#define MAXNUMARGUMENTS 3
#define FLAGLOCATION 1
#define FILELOCATION 2
#define FILELOCATIONBASIC 1
#define DEFAULTFILESIZE 1

char *flag = "-s";
char *hash = "#";
char *deliminator = " ";

void help() {
    printf("Use format as follows: format <name of file to format>\n");
}

//Method that writes the disk image with the given size
void write_disk(char *file_name, int file_size) {

}

/*
 * Parse the command line input and ensure that the user has put in the correct information
 */
boolean parseCmd(char *integer, long long int *MB) {
    //(error checking gotten from stack overflow)
    const char *nptr = integer;                     /* string to read               */
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
    } else if (errno != 0 && number == 0) {
        printf(" number : %lld  invalid  (unspecified error occurred)\n", number);
        return FALSE;
    } else if (errno == 0 && nptr && *endptr != 0) {
        printf(" number : %lld    invalid  (since additional characters remain)\n", number);
        return FALSE;
    }

    //passed all the if statements and so can assigned the seconds value as the one input
    *MB = number;
    return TRUE;
}

//Please note that we are assuming a valid input is given here; specifically, we are assuming a valid number is put
int main(int argc, char *argv[]) {
    printf("number of arguments: %d\n", argc);
    printf("second argument: %s\n", argv[1]);
    printf("third argument: %s\n", argv[2]);

    if (argc != MINNUMARGUMENTS && argc != MAXNUMARGUMENTS) {
        help();
    } else {
        if (argc == MINNUMARGUMENTS) {
            printf("file name: %s\n", argv[FILELOCATIONBASIC]);
            write_disk(argv[FILELOCATIONBASIC], DEFAULTFILESIZE);
        } else {
            char *token = strtok(argv[FLAGLOCATION], deliminator);
            printf("token %s\n", token);

            printf("strcmp value %d\n", strcmp(flag, token));
            if (strcmp(flag, token) == 0) {
                token = strtok(NULL, deliminator);
                printf("token %s\n", token);

                token = strtok(token, hash);
                printf("number of megabytes %s\n", token);

                //convert the token to an integer value
                long long int MB;
                if (parseCmd(token, &MB) == TRUE) {
                    printf("number of megabytes after translation %lld\n", MB);
                } else {
                    help();
                }
            }
        }
    }
}

