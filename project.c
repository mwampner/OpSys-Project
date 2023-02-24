#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <math.h>

int main(int argc, char** argv)
{
    //input validation

    if(argc != 6){
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        return EXIT_FAILURE;
    }

    if(*(argv+1) < 0 || *(argv+1) > 26 || isalpha(*(argv+1))){
        fprintf(stderr, "ERROR: Invalid number of processes\n");
        return EXIT_FAILURE;
    }

    if(*(argv+2) < 0 || *(argv+2) > *(argv+1) || isalpha(*(argv+2))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    if(*(argv+3) < 0 ||  isalpha(*(argv+3))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    if(*(argv+4) < 0 || isalpha(*(argv+4))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    if(*(argv+5) < 0 || isalpha(*(argv+5))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    //assign input variables
    int processes = *(argv+1);
    int cpu_bound = *(argv+2);
    int seed = *(argv+3);
    int interarrival = *(argv+4);
    int upper_bound = *(argv+5);
}