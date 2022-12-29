#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

void help(){
    fprintf(stderr, "Usage: <structure_id> [pid] \n"
            "Supported structure_ids: \n"
            "0 - pci_dev \n"
            "1 - inode [pid]\n");
}

int main(int argc, char *argv[]) {
    // checks
    if (argc != 2 && argc != 3) {
        help();
        fprintf(stderr, "Incorrect amount of arguments \n");
        return -1;
    }

    char *endptr1;
    errno = 0;
    int structure_id = strtol(argv[1], &endptr1, 10);
    if (*endptr1 != '\0' || errno != 0){
        fprintf(stderr, "Structure_id must be a number \n");
        help();
        return -1;
    }

    if (structure_id == 0) {
        if (argc > 2) {
            fprintf(stderr, "Too many arguments \n");
            return -1;
        }
    }
    else if (structure_id == 1) {
        if (argc < 3) {
            fprintf(stderr, "Too few arguments \n");
            return -1;
        }
        char *endptr2;
        errno = 0;
        int pid = strtol(argv[2], &endptr2, 10);
        if (*endptr2 != '\0' || errno != 0){
            fprintf(stderr, "pid must be a number \n");
            help();
            return -1;
         }
        if (pid < 0) {
            fprintf(stderr, "Wrong pid\n");
            return -1;
         }
    }
    else {
        fprintf(stderr, "Provided structure_id is not supported.\n");
        help();
        return -1;
    }

    FILE* fptr = fopen("/proc/lab_driver", "r+");
    if (fptr == NULL) {
        fprintf(stderr, "Cannot find file \n");
        return -1;
    }

    // write
    if (structure_id == 0) {
        fprintf(fptr, "%s %s", argv[1], "");
    }
    else if (structure_id == 1) {
        fprintf(fptr, "%s %s", argv[1], argv[2]);
    }
    fclose(fptr);

    char buffer[200];
    sprintf(buffer, "cat /proc/lab_driver");
    system(buffer);
    return 0;
}
