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

typedef struct{
    char name;
    int type; //1 for CPU & 2 for I/O
    int init_arriv;
    int num_bursts;
    int* cpu_bursts;
    int* io_bursts;
} process;

//pseudo random number generation
int next_exp(int lambda){
    return -log(rand())/lambda;
}

int main(int argc, char** argv)
{
    //input validation

    if(argc != 6){
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        return EXIT_FAILURE;
    }

    if(atoi(*(argv+1)) < 0 || atoi(*(argv+1)) > 26 || isalpha(**(argv+1))){
        fprintf(stderr, "ERROR: Invalid number of processes\n");
        return EXIT_FAILURE;
    }

    if(atoi(*(argv+2)) < 0 || atoi(*(argv+2)) > atoi(*(argv+1)) || isalpha(**(argv+2))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    if(atoi(*(argv+3)) < 0 ||  isalpha(**(argv+3))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    if(atof(*(argv+4)) < 0 || isalpha(**(argv+4))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    if(atoi(*(argv+5)) < 0 || isalpha(**(argv+5))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    //assign input variables
    int num_proc = atoi(*(argv+1));
    int cpu_bound = atoi(*(argv+2));
    int seed = atoi(*(argv+3));
    int interarrival = atof(*(argv+4));
    int upper_bound = atoi(*(argv+5));

    //allocate/assign processes array
    process* processes = calloc(num_proc, sizeof(process));
    
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        ptr->name = 65 + i;
        printf("%c\n", ptr->name);
        if(i >= num_proc-cpu_bound){
            ptr->type = 1;
        }
        else{
            ptr->type = 2;
        }
    }

    //scheduling
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;

        srand48(seed);
        
        //initial arrival time from next_exp()
        int arriv = floor(next_exp(interarrival));
        while(arriv > upper_bound){
            arriv = floor(next_exp(interarrival));
        }

        ptr->init_arriv = arriv;
        
        // find number of bursts
        int bursts = ceil(drand48())*100;
        while(bursts > upper_bound){
            bursts = ceil(drand48())*100;
        }

        ptr->num_bursts = bursts;

        //allocate arrays for cpu and io burst times
        ptr->cpu_bursts = calloc(ptr->num_bursts, sizeof(int));
        ptr->io_bursts = calloc(ptr->num_bursts-1, sizeof(int));

        //find burst times
        for(int j = 0; j < ptr->num_bursts; j++){
            //cpu time
            int cpu = ceil(next_exp(interarrival));
            while(cpu > upper_bound){
                cpu = ceil(next_exp(interarrival));
            }
            //cpu-bound
            int* ptr1 = ptr->cpu_bursts + j;
            if(ptr->type == 1){//cpu
                *ptr1 = cpu*4;
            }
            else{
                *ptr1 = cpu;
            }

            //io-bound
            if(j != ptr->num_bursts-1){
                int* ptr2 = ptr->io_bursts + j;
                int io = ceil(next_exp(interarrival));
                while(io > upper_bound){
                    io = ceil(next_exp(interarrival));
                }
                if(ptr->type == 1){//cpu
                    *ptr2 = io*10/4;
                }
                else{
                    *ptr2 = io*10;
                }
            }
        }

    }

    //print statements
}