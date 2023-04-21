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
float next_exp(float lambda, int upper){
    float val = -log(drand48())/lambda;
    while(val > upper){
        val = -log(drand48())/lambda;
    }

    return val;
}


void FCFS(process* processes, int num_proc, int ts){

    int current_time = 0;
    int current_process = -1;

    int* io_block = calloc(num_proc, sizeof(int));
    int io_block_size = 0;

    //create ready_queue
    int* ready_queue = calloc(num_proc, sizeof(int));
    int rq_size = 0;

    int finished = 0;

    bool term = false;

    //context switches
    int cs_cpu = 0;
    int cs_io = 0;

    float io_waiting_time = 0;
    float io_wait_count = 0 ;

    float io_turnaround_time = 0;
    float cpu_turnarount_time = 0;

    float cpu_waiting_time = 0;
    float cpu_wait_count = 0;

    int context_switch = -1;
    int context_switch_out = -1;

    float cpu_average_cpu = 0;
    float cpu_average_io = 0;
    float count_cpu = 0;
    float count_io = 0;
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        if(ptr->type == 1){
            cpu_wait_count += ptr->num_bursts;
        }else{
            io_wait_count += ptr->num_bursts;
        }
        for(int j = 0; j < ptr->num_bursts; j++){
            if(ptr->type == 1){
                cpu_average_cpu += *(ptr->cpu_bursts + j);
                count_cpu++;
            }else{
                cpu_average_io += *(ptr->cpu_bursts + j);
                count_io++;
            }
        }
    }
    float cpu_average = (cpu_average_cpu + cpu_average_io)/(count_cpu + count_io); 

    printf("time %dms: Simulator started for FCFS [Q <empty>]\n", current_time);
    while (1){

        //there is a current process in the cpu
        if(current_process != -1 && context_switch == -1 && context_switch_out == -1){
            process* current_ptr = processes + current_process;
            //cpu burst decrease 
            (*(current_ptr->cpu_bursts+0))--;

            //CPU burst completeion
            if (*(current_ptr->cpu_bursts+0) == 0) {
            
                //block process on IO
                if (current_ptr->num_bursts-1 > 0) {

                    //print 
                    if(current_time < 10000){
                        printf("time %dms: Process %c completed a CPU burst; %d burst%sto go [Q ", current_time , current_ptr->name,current_ptr->num_bursts - 1,(current_ptr->num_bursts - 1) > 1 ? "s " : " ");
                        for (int j = 0; j < rq_size; j++) {
                            process* ready_ptr = processes + *(ready_queue + j);
                            printf("%c", ready_ptr->name);
                            if (j < rq_size - 1) {
                                printf(" ");
                            }
                        }
                        if (rq_size == 0) {
                            printf("<empty>");
                        }
                        printf("]\n");
                    }
                    
                    //move to next cpu burst
                    for(int i=0; i<current_ptr->num_bursts-1 ;i++){
                        *(current_ptr->cpu_bursts + i) = *(current_ptr->cpu_bursts + (i+1));
                    } *(current_ptr->cpu_bursts + current_ptr->num_bursts-1)=0;

                    //print
                    if(current_time < 10000){
                        printf("time %dms: Process %c switching out of CPU; blocking on I/O until time %dms [Q ", current_time, current_ptr->name, current_time + *(current_ptr->io_bursts + 0)+ts);
                        for (int j = 0; j < rq_size; j++) {
                            process* ready_ptr = processes + *(ready_queue + j);
                            printf("%c", ready_ptr->name);
                            if (j < rq_size - 1) {
                                printf(" ");
                            }
                        }
                        if (rq_size == 0) {
                            printf("<empty>");
                        }
                        printf("]\n"); 
                    }
                    context_switch_out = ts; 
                }
                //process terminated
                else{
                    
                    printf("time %dms: Process %c terminated [Q ",current_time, current_ptr->name);
                    for (int j = 0; j < rq_size; j++) {
                        process* ready_ptr = processes + *(ready_queue + j);
                        printf("%c", ready_ptr->name);
                        if (j < rq_size - 1) {
                            printf(" ");
                        }
                    }
                    if (rq_size == 0) {
                        printf("<empty>");
                    }
                    printf("]\n");
                    finished++;
                    //current_process = -1;
                    context_switch_out = ts-1;
                    term = true;
                }
            }
        }
        //new process to start using the CPU
        else if(current_process == -1 && rq_size > 0 && context_switch == -1 && context_switch_out == -1){

            //change current process to next in ready queue
            current_process = *(ready_queue+0);

            for(int i=0; i<rq_size; i++){
                    *(ready_queue + i) = *(ready_queue + (i+1));
            }
            *(ready_queue + rq_size) = 0;
            rq_size--;
            context_switch = ts-1;
        }
        //all process complete
        else if (rq_size == 0 && current_process == -1 && finished == num_proc){
            //print
            printf("time %dms: Simulator ended for FCFS [Q ", current_time);
             for (int j = 0; j < rq_size; j++) {
                process* ready_ptr = processes + *(ready_queue + j);
                printf("%c", ready_ptr->name);
                if (j < rq_size - 1) {
                    printf(" ");
                }
            }
            if (rq_size == 0) {
                printf("<empty>");
            }
            printf("]\n\n");
            break;
        }
        //in context switch into cpu
        else if(context_switch != -1){
            process* current_ptr = processes + current_process; 
            context_switch--;
            //done with context switch
            if(context_switch == 0){
                if(current_ptr->type == 1){
                    cs_cpu++;
                }else{
                    cs_io++;
                }
                //print
                if(current_time < 10000){
                    printf("time %dms: Process %c started using the CPU for %dms burst [Q ", current_time, current_ptr->name, *(current_ptr->cpu_bursts+0));
                    
                    for (int j = 0; j < rq_size; j++) {
                        process* ready_ptr = processes + *(ready_queue + j);
                        printf("%c", ready_ptr->name);
                        if (j < rq_size - 1) {
                            printf(" ");
                        }
                    }
                    if (rq_size == 0) {
                        printf("<empty>");
                    }
                    printf("]\n");
                }
                context_switch = -1;

            }
        }
        //out context switch out of cpu
        else if(context_switch_out != -1){
            context_switch_out--;
            process* ptr = processes + current_process;
            //calculate turnaround
            if(ptr->type == 1){
                cpu_turnarount_time++;
            }else{
                io_turnaround_time++;
            }

            //done with context switch
            if(context_switch_out == 0 && term == false){
                if(ptr->type == 1){
                    cpu_turnarount_time++;
                }else{
                    io_turnaround_time++;
                }
                *(io_block + io_block_size) = current_process;
                io_block_size++; 
                context_switch_out = -1;
                //reset current process
                process* io_ptr = processes + current_process;
                (*(io_ptr->io_bursts+0))++;
                current_process = -1;
            }else if (context_switch_out == 0 && term == true){
                term = false;
                current_process = -1;
            }
        }

        //calculate wait time
        for(int i=0; i<rq_size; i++){
            process* ptr = processes + *(ready_queue + i);
            if(ptr->type == 1){
                cpu_waiting_time++;
                cpu_turnarount_time++;
            }else{
                io_waiting_time++;
                io_turnaround_time++;
            }
        }

        //check for new arrivals 
        for(int i=0; i<num_proc; i++){
            process* ptr = processes + i;
            if (ptr->init_arriv == current_time) {
                //print
                if(current_time < 10000){
                    printf("time %dms: Process %c arrived; added to ready queue [Q ", current_time, ptr->name);
                    *(ready_queue + rq_size) = i;
                    rq_size++;
                    for (int j = 0; j < rq_size; j++) {
                        process* ptr2 = processes + (*(ready_queue+j));
                        printf("%c", ptr2->name);
                        if (j < rq_size - 1) {
                            printf(" ");
                        }
                    }
                    printf("]\n");
                }
            }
        }
        //check IO block completions
        for(int s=0; s<num_proc; s++){
            for(int i=0; i<io_block_size; i++){
                if(s == *(io_block + i)){
                    process* io_ptr = processes + s;
                    (*(io_ptr->io_bursts+0))--;
                    if (*(io_ptr->io_bursts+0) == 0){
                        io_ptr->num_bursts--;

                        //print
                        if(current_time < 10000){
                            printf("time %dms: Process %c completed I/O; added to ready queue [Q ", current_time, io_ptr->name);
                        }

                        *(ready_queue+rq_size) = s;
                        rq_size++;
                        //print
                        if(current_time < 10000){
                            for (int j = 0; j < rq_size; j++) {
                                process* ready_ptr = processes + *(ready_queue + j);
                                printf("%c", ready_ptr->name);
                                if (j < rq_size - 1) {
                                    printf(" ");
                                }
                            }
                            printf("]\n");
                        }

                        for(int s=i; s<io_block_size-1;s++){
                            *(io_block + s) = *(io_block + (s+1));
                        }*(io_block + io_block_size-1) = 0;
                        io_block_size--;

                        for(int j=0; j<io_ptr->num_bursts-1; j++){
                            *(io_ptr->io_bursts+j) = *(io_ptr->io_bursts+(j+1));
                        }*(io_ptr->io_bursts + (io_ptr->num_bursts-1)) = 0;
                        i--;
                    }
                }
            }
        }

        //increment time
        current_time++; 
    }
    

    free(ready_queue);
    free(io_block);

    FILE *fp;
    fp = fopen("simout.txt" ,"a");
    
    fprintf(fp, "Algorithm FCFS\n");
    float use = ((cpu_average_cpu + cpu_average_io) / current_time)*100;
    fprintf(fp, "-- CPU utilization: %.3f%%\n", ceil(use*1000.0)/1000);
    use = (cpu_average_cpu/count_cpu);
    float use2 = (cpu_average_io/count_io);
    fprintf(fp, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n",(ceil(cpu_average*1000.0)/1000),(ceil(use*1000.0)/1000),(ceil(use2*1000.0)/1000));
    use = ((io_waiting_time + cpu_waiting_time)/(io_wait_count + cpu_wait_count));
    use2 = (cpu_waiting_time/cpu_wait_count);
    float use3 = (io_waiting_time/io_wait_count);
    fprintf(fp, "-- average wait time:  %.3f ms ( %.3f ms/ %.3f ms)\n", (ceil(use*1000.0)/1000), (ceil(use2*1000.0)/1000),(ceil(use3*1000.0)/1000));
    use = ((cpu_average_cpu + cpu_average_io + (count_io*(ts*2)) + (count_cpu*(ts*2)) + io_waiting_time + cpu_waiting_time)/(count_cpu + count_io));
    use2 = ((cpu_average_cpu + (count_cpu*(ts*2)) + cpu_waiting_time)/(count_cpu));
    use3 = ((cpu_average_io + (count_io*(ts*2)) + io_waiting_time )/(count_io));
    fprintf(fp, "-- average turnaround time:  %.3f ms ( %.3f ms/ %.3f ms)\n",(ceil(use*1000.0)/1000), (ceil(use2*1000.0)/1000),(ceil(use3*1000.0)/1000));
    fprintf(fp, "-- number of context switches: %d (%d/%d)\n", cs_cpu + cs_io, cs_cpu, cs_io);
    fprintf(fp, "-- number of preemptions: 0 (0/0)\n\n");

    fclose(fp);
    return;
    

    return;
}

void RR(process* processes, int num_proc, int ts, int time_slice){
    int current_time = 0;
    int current_process = -1;

    int time_left = time_slice;

    int* io_block = calloc(num_proc, sizeof(int));
    int io_block_size = 0;

    //create ready_queue
    int* ready_queue = calloc(num_proc, sizeof(int));
    int rq_size = 0;

    int finished = 0;

    //context switches
    int cs_cpu = 0;
    int cs_io = 0;

    int num_preumptions_cpu = 0;
    int num_preumptions_io = 0;

    bool preump = false;
    bool term = false;
    int* current_max_burst = calloc(num_proc,sizeof(int));

    float io_waiting_time = 0;
    float io_wait_count = 0 ;

    float io_turnaround_time = 0;
    float cpu_turnarount_time = 0;

    float cpu_waiting_time = 0;
    float cpu_wait_count = 0;

    int context_switch = -1;
    int context_switch_out = -1;

    float cpu_average_cpu = 0;
    float cpu_average_io = 0;
    float count_cpu = 0;
    float count_io = 0;
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        if(ptr->type == 1){
            cpu_wait_count += ptr->num_bursts;
        }else{
            io_wait_count += ptr->num_bursts;
        }
        for(int j = 0; j < ptr->num_bursts; j++){
            if(ptr->type == 1){
                cpu_average_cpu += *(ptr->cpu_bursts + j);
                count_cpu++;
            }else{
                cpu_average_io += *(ptr->cpu_bursts + j);
                count_io++;
            }
        }
    }
    float cpu_average = (cpu_average_cpu + cpu_average_io)/(count_cpu + count_io); 

    printf("time %dms: Simulator started for RR [Q <empty>]\n", current_time);
    while (1){

        //there is a current process in the cpu
        if(current_process != -1 && context_switch == -1 && context_switch_out == -1){
            process* current_ptr = processes + current_process;
            //cpu burst decrease 
            (*(current_ptr->cpu_bursts+0))--;
            time_left--;
            //printf("%d\n", time_left);

            //CPU burst completeion
            if (*(current_ptr->cpu_bursts+0) == 0) {
            
                //block process on IO
                if (current_ptr->num_bursts-1 > 0) {
                    if(current_time < 10000){
                        printf("time %dms: Process %c completed a CPU burst; %d burst%sto go [Q ", current_time , current_ptr->name,current_ptr->num_bursts - 1,(current_ptr->num_bursts - 1) > 1 ? "s " : " ");
                        for (int j = 0; j < rq_size; j++) {
                            process* ready_ptr = processes + *(ready_queue + j);
                            printf("%c", ready_ptr->name);
                            if (j < rq_size - 1) {
                                printf(" ");
                            }
                        }
                        if (rq_size == 0) {
                            printf("<empty>");
                        }
                        printf("]\n");
                    }

                    //move to next cpu burst
                    for(int i=0; i<current_ptr->num_bursts-1 ;i++){
                        *(current_ptr->cpu_bursts + i) = *(current_ptr->cpu_bursts + (i+1));
                    } *(current_ptr->cpu_bursts + current_ptr->num_bursts-1)=0;

                    if(current_time < 10000){
                        printf("time %dms: Process %c switching out of CPU; blocking on I/O until time %dms [Q ", current_time, current_ptr->name, current_time + *(current_ptr->io_bursts + 0)+ts);
                        for (int j = 0; j < rq_size; j++) {
                            process* ready_ptr = processes + *(ready_queue + j);
                            printf("%c", ready_ptr->name);
                            if (j < rq_size - 1) {
                                printf(" ");
                            }
                        }
                        if (rq_size == 0) {
                            printf("<empty>");
                        }
                        printf("]\n"); 
                    }
                    context_switch_out = ts; 
                    preump = false;

                }
                //process terminated
                else{
                    printf("time %dms: Process %c terminated [Q ",current_time, current_ptr->name);
                    for (int j = 0; j < rq_size; j++) {
                        process* ready_ptr = processes + *(ready_queue + j);
                        printf("%c", ready_ptr->name);
                        if (j < rq_size - 1) {
                            printf(" ");
                        }
                    }
                    if (rq_size == 0) {
                        printf("<empty>");
                    }
                    printf("]\n");
                    finished++;
                    //context_switch_out = ts;
                    preump = true;
                    //current_process = -1;
                    context_switch_out = ts-1;
                    term = true;
                }
                //reset time slice
                time_left = time_slice;
            }//time run out
            else if(time_left == 0){
                //reset time slice
                time_left = time_slice;

                //no premptions occur
                if(rq_size == 0){
                    if(current_time < 10000){
                        printf("time %dms: Time slice expired; no preemption because ready queue is empty [Q ", current_time);
                        for (int j = 0; j < rq_size; j++) {
                            process* ready_ptr = processes + *(ready_queue + j);
                            printf("%c", ready_ptr->name);
                            if (j < rq_size - 1) {
                                printf(" ");
                            }
                        }
                        if (rq_size == 0) {
                            printf("<empty>");
                        }
                        printf("]\n");
                    }

                }//preumpt process
                else{
                    if(current_ptr->type == 1){
                        num_preumptions_cpu++;
                    }else{
                        num_preumptions_io++;
                    }
                    
                    if(current_time < 10000){
                        printf("time %dms: Time slice expired; preempting process %c with %dms remaining [Q ", current_time, current_ptr->name, (*(current_ptr->cpu_bursts+0)));
                        for (int j = 0; j < rq_size; j++) {
                            process* ready_ptr = processes + *(ready_queue + j);
                            printf("%c", ready_ptr->name);
                            if (j < rq_size - 1) {
                                printf(" ");
                            }
                        }
                        if (rq_size == 0) {
                            printf("<empty>");
                        }
                        printf("]\n");
                    }
                    
                    //current_process = -1;
                    context_switch_out = ts; 
                    preump = true;
                }
            }
        }
        //new process to start using the CPU
        else if(current_process == -1 && rq_size > 0 && context_switch == -1 && context_switch_out == -1){

            //change current process to next in ready queue
            current_process = *(ready_queue+0);

            for(int i=0; i<rq_size; i++){
                    *(ready_queue + i) = *(ready_queue + (i+1));
            }
            *(ready_queue + rq_size) = 0;
            rq_size--;
            context_switch = ts-1;
        }
        //all process complete
        else if (rq_size == 0 &&  finished == num_proc && current_process == -1){// && context_switch == -1 && context_switch_out == -1){
            //current_time++;
            printf("time %dms: Simulator ended for RR [Q ", current_time);
             for (int j = 0; j < rq_size; j++) {
                process* ready_ptr = processes + *(ready_queue + j);
                printf("%c", ready_ptr->name);
                if (j < rq_size - 1) {
                    printf(" ");
                }
            }
            if (rq_size == 0) {
                printf("<empty>");
            }
            printf("]\n");
            break;
        }
        //in context switch into cpu
        else if(context_switch != -1){
            process* current_ptr = processes + current_process; 
            context_switch--;
            //done with context switch
            if(context_switch == 0){
                if(current_ptr->type == 1){
                    cs_cpu++;
                }else{
                    cs_io++;
                }
                if(current_time < 10000){
                    if(*(current_max_burst + current_process) == *(current_ptr->cpu_bursts + 0)){
                        printf("time %dms: Process %c started using the CPU for %dms burst [Q ", current_time, current_ptr->name, *(current_ptr->cpu_bursts+0));
                    }else{
                        printf("time %dms: Process %c started using the CPU for remaining %dms of %dms burst [Q ", current_time, current_ptr->name, *(current_ptr->cpu_bursts+0), *(current_max_burst + current_process));
                    }
                    
                    
                    for (int j = 0; j < rq_size; j++) {
                        process* ready_ptr = processes + *(ready_queue + j);
                        printf("%c", ready_ptr->name);
                        if (j < rq_size - 1) {
                            printf(" ");
                        }
                    }
                    if (rq_size == 0) {
                        printf("<empty>");
                    }
                    printf("]\n");
                }
                context_switch = -1;
            }
        }
        //out context switch out of cpu
        else if(context_switch_out != -1){
            context_switch_out--;
            process* ptr = processes + current_process;
            //calculate turnaround
            if(ptr->type == 1){
                cpu_turnarount_time++;
            }else{
                io_turnaround_time++;
            }

            //done with context switch
            if(context_switch_out == 0 && preump == false && term == false){
                if(ptr->type == 1){
                    cpu_turnarount_time++;
                }else{
                    io_turnaround_time++;
                }
                *(io_block + io_block_size) = current_process;
                io_block_size++; 
                context_switch_out = -1;
                //reset current process
                process* io_ptr = processes + current_process;
                (*(io_ptr->io_bursts+0))++;
                current_process = -1;
            }else if(context_switch_out == 0 && preump == true && term == false){
                if(ptr->type == 1){
                    cpu_turnarount_time++;
                }else{
                    io_turnaround_time++;
                }
                *(ready_queue + rq_size) = current_process;
                rq_size++;
                current_process = -1;
                context_switch_out = -1;
            }
            else if (context_switch_out == 0 && term == true){
                term = false;
                current_process = -1;
            }
        }

        //calculate wait time
        for(int i=0; i<rq_size; i++){
            process* ptr = processes + *(ready_queue + i);
            if(ptr->type == 1){
                cpu_waiting_time++;
                cpu_turnarount_time++;
            }else{
                io_waiting_time++;
                io_turnaround_time++;
            }
        }

        //check for new arrivals 
        for(int i=0; i<num_proc; i++){
            process* ptr = processes + i;
            if (ptr->init_arriv == current_time) {
                if(current_time < 10000){
                    printf("time %dms: Process %c arrived; added to ready queue [Q ", current_time, ptr->name);
                }
                *(ready_queue + rq_size) = i;
                rq_size++;
                if(current_time < 10000){
                    for (int j = 0; j < rq_size; j++) {
                        process* ptr2 = processes + (*(ready_queue+j));
                        printf("%c", ptr2->name);
                        if (j < rq_size - 1) {
                            printf(" ");
                        }
                    }
                    printf("]\n");
                }
                *(current_max_burst + i) = *(ptr->cpu_bursts + 0);
            }
        }
        //check IO block completions
        for(int s=0; s<num_proc; s++){
            for(int i=0; i<io_block_size; i++){
                if(s == *(io_block + i)){    
                    process* io_ptr = processes + s;
                    (*(io_ptr->io_bursts+0))--;
                    if (*(io_ptr->io_bursts+0) == 0){
                        io_ptr->num_bursts--;
                        if(current_time < 10000){
                            printf("time %dms: Process %c completed I/O; added to ready queue [Q ", current_time, io_ptr->name);
                        }

                        *(ready_queue+rq_size) = s;
                        rq_size++;
                        if(current_time < 10000){
                            for (int j = 0; j < rq_size; j++) {
                                process* ready_ptr = processes + *(ready_queue + j);
                                printf("%c", ready_ptr->name);
                                if (j < rq_size - 1) {
                                    printf(" ");
                                }
                            }
                            printf("]\n");
                        }
                        for(int s=i; s<io_block_size-1;s++){
                            *(io_block + s) = *(io_block + (s+1));
                        }*(io_block + io_block_size-1) = 0;
                        io_block_size--;

                        for(int j=0; j<io_ptr->num_bursts-1; j++){
                            *(io_ptr->io_bursts+j) = *(io_ptr->io_bursts+(j+1));
                        }*(io_ptr->io_bursts + (io_ptr->num_bursts-1)) = 0;
                        i--;
                        *(current_max_burst + s) = *(io_ptr->cpu_bursts + 0);
                    }
                }
            }
        }

        //increment time
        current_time++;        
    }
    free(ready_queue);
    free(io_block);
    free(current_max_burst);

    FILE *fp;
    fp = fopen("simout.txt" ,"a");
    
    fprintf(fp, "Algorithm RR\n");
    float use = ((cpu_average_cpu + cpu_average_io) / current_time)*100;
    fprintf(fp, "-- CPU utilization: %.3f%%\n", ceil(use*1000.0)/1000);
    use = (cpu_average_cpu/count_cpu);
    float use2 = (cpu_average_io/count_io);
    fprintf(fp, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n",(ceil(cpu_average*1000.0)/1000),(ceil(use*1000.0)/1000),(ceil(use2*1000.0)/1000));
    use = ((io_waiting_time + cpu_waiting_time)/(io_wait_count + cpu_wait_count));
    use2 = (cpu_waiting_time/cpu_wait_count);
    float use3 = (io_waiting_time/io_wait_count);
    fprintf(fp, "-- average wait time:  %.3f ms ( %.3f ms/ %.3f ms)\n", (ceil(use*1000.0)/1000), (ceil(use2*1000.0)/1000),(ceil(use3*1000.0)/1000));
    use = ((cpu_average_cpu + cpu_average_io + (ts*(count_io + num_preumptions_io)) + (ts*(count_cpu + num_preumptions_cpu)) + io_waiting_time + cpu_waiting_time)/(count_cpu + count_io));
    use2 = ((cpu_average_cpu + (ts*(count_cpu + num_preumptions_cpu)) + cpu_waiting_time)/(count_io));
    use3 =((cpu_average_io + (ts*(count_io + num_preumptions_io)) + io_waiting_time)/(count_cpu));
    fprintf(fp, "-- average turnaround time:  %.3f ms ( %.3f ms/ %.3f ms)\n",(ceil(use*1000.0)/1000), (ceil(use2*1000.0)/1000),(ceil(use3*1000.0)/1000));
    fprintf(fp, "-- number of context switches: %d (%d/%d)\n", cs_cpu + cs_io, cs_cpu, cs_io);
    fprintf(fp, "-- number of preemptions: %d (%d/%d)\n", num_preumptions_io + num_preumptions_cpu, num_preumptions_cpu, num_preumptions_io);

    fclose(fp);
    return;
}




int main(int argc, char** argv)
{
    //input validation
    if(argc != 9){
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

    if(atoi(*(argv+6)) < 0 || isalpha(**(argv+5))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    if(atof(*(argv+7)) < 0 || isalpha(**(argv+5))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    if(atoi(*(argv+8)) < 0 || isalpha(**(argv+5))){
        fprintf(stderr, "ERROR: Invalid number of CPU-bound processes\n");
        return EXIT_FAILURE;
    }

    //assign input variables
    int num_proc = atoi(*(argv+1));
    int cpu_bound = atoi(*(argv+2));
    int seed = atoi(*(argv+3));
    float interarrival = atof(*(argv+4));
    int upper_bound = atoi(*(argv+5));
    int time_cs = atoi(*(argv+6));
    //float alpha =  atof(*(argv+7));
    int t_slice =  atoi(*(argv+8));

    //allocate/assign processes array
    process* processes = calloc(num_proc, sizeof(process));
    
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        ptr->name = 65 + i;
        if(i >= num_proc-cpu_bound){
            ptr->type = 1;
        }
        else{
            ptr->type = 2;
        }
    }

    //scheduling
    srand48(seed);
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        
        //initial arrival time from next_exp()
        int arriv = floor(next_exp(interarrival, upper_bound));
        //printf("ARRIVAL TIME: %d\n", arriv);
        ptr->init_arriv = arriv;
        
        // find number of bursts
        int bursts = ceil(drand48()*100);
        while(bursts > upper_bound){
            bursts = ceil(drand48()*100);
        }

        ptr->num_bursts = bursts;

        //allocate arrays for cpu and io burst times
        ptr->cpu_bursts = calloc(ptr->num_bursts, sizeof(int));
        ptr->io_bursts = calloc(ptr->num_bursts-1, sizeof(int));

        //find burst times
        for(int j = 0; j < ptr->num_bursts; j++){
            //cpu time
            int cpu = ceil(next_exp(interarrival, upper_bound));

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
                int io = ceil(next_exp(interarrival, upper_bound));

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
    printf("<<< PROJECT PART I -- process set (n=%d) with %d CPU-bound ", num_proc, cpu_bound);
    if(cpu_bound == 1){
        printf("process >>>\n");
    }else{
        printf("processes >>>\n");
    }
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        if(ptr->type == 1){
            printf("CPU");
        }
        else{
            printf("I/O");
        }
        printf("-bound process %c: arrival time %dms; %d CPU bursts\n", ptr->name, ptr->init_arriv, ptr->num_bursts);
/*
        for(int j = 0; j < ptr->num_bursts; j++){
            printf("--> CPU burst %dms", *(ptr->cpu_bursts+j));
            
            if(j < ptr->num_bursts-1){
                printf(" --> I/O burst %dms", *(ptr->io_bursts+j));
            }

            printf("\n");
        }*/
    }
    printf("\n");



    //copy processes for FCFS
    process* processesFCFS = calloc(num_proc, sizeof(process));
    
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        process* ptr2 = processesFCFS + i;
        ptr2->name = ptr->name;
        ptr2->type = ptr->type;
        ptr2->init_arriv = ptr->init_arriv;
        ptr2->num_bursts = ptr->num_bursts;
        ptr2->cpu_bursts = calloc(ptr->num_bursts, sizeof(int));
        ptr2->io_bursts = calloc(ptr->num_bursts-1, sizeof(int));
        for(int j=0; j<ptr->num_bursts-1; j++){
            *(ptr2->cpu_bursts + j) = *(ptr->cpu_bursts + j);
            *(ptr2->io_bursts + j) = *(ptr->io_bursts + j);
        }*(ptr2->cpu_bursts + (ptr->num_bursts-1)) = *(ptr->cpu_bursts + (ptr->num_bursts-1));
    }

    process* processesRR = calloc(num_proc, sizeof(process));
    
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        process* ptr2 = processesRR + i;
        ptr2->name = ptr->name;
        ptr2->type = ptr->type;
        ptr2->init_arriv = ptr->init_arriv;
        ptr2->num_bursts = ptr->num_bursts;
        ptr2->cpu_bursts = calloc(ptr->num_bursts, sizeof(int));
        ptr2->io_bursts = calloc(ptr->num_bursts-1, sizeof(int));
        for(int j=0; j<ptr->num_bursts-1; j++){
            *(ptr2->cpu_bursts + j) = *(ptr->cpu_bursts + j);
            *(ptr2->io_bursts + j) = *(ptr->io_bursts + j);
        }*(ptr2->cpu_bursts + (ptr->num_bursts-1)) = *(ptr->cpu_bursts + (ptr->num_bursts-1));
    }
    
    


    FCFS(processesFCFS, num_proc, time_cs/2);
    RR(processesRR, num_proc, time_cs/2, t_slice);
    

    for(int i=0; i<num_proc; i++){
        process* ptr = processesFCFS + i;
        free(ptr->io_bursts);
        free(ptr->cpu_bursts);
    }
    free(processesFCFS);

    for(int i=0; i<num_proc; i++){
        process* ptr = processesRR + i;
        free(ptr->io_bursts);
        free(ptr->cpu_bursts);
    }
    free(processesRR);

    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        free(ptr->cpu_bursts);
        free(ptr->io_bursts);
    }
    free(processes);


}
