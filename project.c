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
    char status;
    int burst_est;
    int init_arriv;
    int num_bursts;
    int bursts_remaining;
    int* cpu_bursts;
    int* io_bursts;
    int return_time;
    int last_use;
    int* turnaround_time;
    int* wait_time;
    int preempt;
    bool preed;
    int prev;
    bool fut_pre;
    
} process;



//pseudo random number generation
float next_exp(float lambda, int upper){
    float val = -log(drand48())/lambda;
    while(val > upper){
        val = -log(drand48())/lambda;
    }

    return val;
}

char * get_queue(int num_waiting, process * waiting){
    char* q;
    if(num_waiting != 0){
        q = calloc(4 + 2*num_waiting, sizeof(char));
        int q_in = 0;
        *(q+q_in) = '[';
        q_in++;
        *(q+q_in) = 'Q';
        q_in++;
        for(int i = 0; i < num_waiting; i++){
            *(q+q_in) = ' ';
            q_in++;
            *(q+q_in) = (*(waiting+i)).name;
            q_in++;
        }
        *(q+q_in) = ']';
        q_in++;
        *(q+q_in) = '\0';
        
    }
    else{
        //q = calloc(12, sizeof(char));
        q = "[Q <empty>]\0";
    }
    return q;
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

            for(int i=0; i<rq_size-1; i++){
                    *(ready_queue + i) = *(ready_queue + (i+1));
            }
            if(rq_size< num_proc){
                *(ready_queue + rq_size) = 0;
            }
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
    fprintf(fp, "-- CPU utilization: %.3f%%\n", ceil(use*1000)/1000);
    use = (cpu_average_cpu/count_cpu);
    float use2 = (cpu_average_io/count_io);
    fprintf(fp, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n",(ceil(cpu_average*1000)/1000),(ceil(use*1000)/1000),(ceil(use2*1000)/1000));
    use = ((io_waiting_time + cpu_waiting_time)/(io_wait_count + cpu_wait_count));
    use2 = (cpu_waiting_time/cpu_wait_count);
    float use3 = (io_waiting_time/io_wait_count);
    fprintf(fp, "-- average wait time:  %.3f ms ( %.3f ms/ %.3f ms)\n", (ceil(use*1000)/1000), (ceil(use2*1000)/1000),(ceil(use3*1000)/1000));
    use = ((cpu_average_cpu + cpu_average_io + (count_io*(ts*2)) + (count_cpu*(ts*2)) + io_waiting_time + cpu_waiting_time)/(count_cpu + count_io));
    use2 = ((cpu_average_cpu + (count_cpu*(ts*2)) + cpu_waiting_time)/(count_cpu));
    use3 = ((cpu_average_io + (count_io*(ts*2)) + io_waiting_time )/(count_io));
    fprintf(fp, "-- average turnaround time:  %.3f ms ( %.3f ms/ %.3f ms)\n",(ceil(use*1000)/1000), (ceil(use2*1000)/1000),(ceil(use3*1000)/1000));
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

            for(int i=0; i<rq_size-1; i++){
                    *(ready_queue + i) = *(ready_queue + (i+1));
            }
            if(rq_size < num_proc){
                *(ready_queue + rq_size) = 0;
            }
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
                    cpu_waiting_time--;
                }else{
                    io_waiting_time--;
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
    fprintf(fp, "-- CPU utilization: %.3f%%\n", ceil(use*1000)/1000);
    use = (cpu_average_cpu/count_cpu);
    float use2 = (cpu_average_io/count_io);
    fprintf(fp, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n",(ceil(cpu_average*1000)/1000),(ceil(use*1000)/1000),(ceil(use2*1000)/1000));
    use = ((io_waiting_time + cpu_waiting_time)/(io_wait_count + cpu_wait_count));
    use2 = (cpu_waiting_time/cpu_wait_count);
    float use3 = (io_waiting_time/io_wait_count);
    fprintf(fp, "-- average wait time:  %.3f ms ( %.3f ms/ %.3f ms)\n", (ceil(use*1000)/1000), (ceil(use2*1000)/1000),(ceil(use3*1000)/1000));
    use = ((cpu_average_cpu + cpu_average_io + (((2*ts)*count_cpu) + ((ts*2)*num_preumptions_cpu)) + (((2*ts)*count_io) + ((ts*2) * num_preumptions_io)) + io_waiting_time + cpu_waiting_time)/(count_cpu + count_io));
    use2 = ((cpu_average_cpu + ((2*ts)*count_cpu) + ((ts*2)*num_preumptions_cpu)) + cpu_waiting_time)/(count_cpu);
    use3 =(cpu_average_io + ((2*ts)*count_io) + ((ts*2) * num_preumptions_io) + io_waiting_time)/(count_io);
    fprintf(fp, "-- average turnaround time:  %.3f ms ( %.3f ms/ %.3f ms)\n",(ceil(use*1000)/1000), (ceil(use2*1000)/1000),(ceil(use3*1000)/1000));
    fprintf(fp, "-- number of context switches: %d (%d/%d)\n", cs_cpu + cs_io, cs_cpu, cs_io);
    fprintf(fp, "-- number of preemptions: %d (%d/%d)\n", num_preumptions_io + num_preumptions_cpu, num_preumptions_cpu, num_preumptions_io);

    fclose(fp);
    return;
}

void sjf(process * unarrived, int len, int t_cs, float est, int upper){
    process running;
    float sys_burst = 0;
    running.name = 0;
    process * io = calloc(len, sizeof(process));
    process * waiting = calloc(len, sizeof(process));
    process * finished = calloc(len, sizeof(process));
    int f_num = 0;
    int num_waiting = 0;
    int num_io = 0;
    int time = 0;
    int first = -1;

    printf("time 0ms: Simulator started for SJF [Q <empty>]\n");
    //find earliest arrivel
    for(int i = 0; i < len; i++){
        if(((unarrived+i)->init_arriv < first || first == -1) && (unarrived+i)->status == 'u'){
            first = (unarrived+i)->init_arriv;
        }
    }
    time = first;

    //get initial waiting processes
    for(int i = 0; i < len; i++){
        if((unarrived+i)->init_arriv == first && (unarrived+i)->status == 'u'){
           *(waiting + num_waiting) = *(unarrived+i);
           num_waiting ++;
            //queue string
           char* queue = get_queue(num_waiting, waiting);
            printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", time, (*(unarrived+i)).name, (*(unarrived+i)).burst_est, queue);
            (unarrived+i)->status = 'w';
            if(num_waiting != 0){
                free(queue);
            }
            (*(waiting +num_waiting)).last_use = time;
        }
    }

    //get first process
    first = -1;
    for(int i = 0; i < num_waiting; i++){
        if((waiting+i)->burst_est < first || first == -1){
            running = *(waiting+i);
        }
    }
    num_waiting--;
    time += t_cs/2;

    //need to check for other arrivals 
    char* queue = get_queue(num_waiting, waiting);
    printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", time, running.name, running.burst_est, *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining)), queue);
    sys_burst += *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining));
    *(running.turnaround_time+(running.num_bursts-running.bursts_remaining)) = *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining)) + t_cs/2 + (time - running.init_arriv);
    *(running.wait_time+(running.num_bursts-running.bursts_remaining)) = time - t_cs - running.last_use;
    if(num_waiting != 0){
        free(queue);
    }
    running.status = 'r';

    //sjf algorithm
    while(f_num < len){
        int burst = *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining));
        bool event = true;
        int event_time = -1;
        process event_proc;
        //find newest arrivals or io if current CPU burst exceeds arrival time
        //printf("TIME: %d \n", time+burst);
        while(event){
            event_time = -1;
            event = false;
            int num = 0;
            //find arrival
            for(int i = 0; i < len; i++){
                if(event_time != -1 && (*(unarrived+i)).status == 'u' && (*(unarrived+i)).return_time == event_time && (*(unarrived+i)).name < event_proc.name){
                    event = true;
                    event_time = (*(unarrived+i)).init_arriv;
                    event_proc = (*(unarrived+i));
                    num = i;
                }
                else if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+burst && ((unarrived+i)->init_arriv < event_time || event_time == -1)){
                    //printf("NAME: %c !!!!\n", (*(unarrived+i)).name);
                    event = true;
                    event_time = (*(unarrived+i)).init_arriv;
                    event_proc = (*(unarrived+i));
                    num = i;
                }
            }
            //find io finish
            for(int i = 0; i < num_io; i++){
                if((*(io+i)).return_time < time+burst && ((*(io+i)).return_time < event_time || event_time == -1)){ 
                    event = true;
                    event_time = (*(io+i)).return_time;
                    event_proc = (*(io+i));
                    num = i;
                }
                else if((*(io+i)).return_time == event_time && event_proc.status == 'i' && (*(io+i)).name < event_proc.name){
                    event = true;
                    event_time = (*(io+i)).return_time;
                    event_proc = (*(io+i));
                    num = i;
                }
            }
            if(event){
                //add to waiting list
                int index = num_waiting;
                if(num_waiting != 0){
                    for(int j = 0; j < num_waiting; j++){
                        if(event_proc.burst_est < (*(waiting+j)).burst_est){
                            index = j;
                            j = num_waiting;
                        }
                        else if(event_proc.burst_est == (*(waiting+j)).burst_est && event_proc.name < (*(waiting+j)).name){
                            index = j;
                            j = num_waiting;
                        }
                    }
                    //num_waiting++;
                    if(index != num_waiting){
                        for(int j = num_waiting; j > index; j--){
                            //printf("NAME: %c!!!\n", (*(waiting+j)).name);
                            *(waiting+j) = *(waiting+j-1);
                        }
                        *(waiting+index) = event_proc;
                    }
                }
                num_waiting++;
                *(waiting+index) = event_proc;
                //printf("NAME: %c\n", event_proc.name);
                //print messages
                char* queue = get_queue(num_waiting, waiting);
                if(((event_proc)).status == 'u'){
                    (*(unarrived+num)).status = 'w';
                    (*(waiting+index)).last_use = (*(waiting+index)).init_arriv;
                    if(event_proc.init_arriv <= 9999){
                        printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", event_proc.init_arriv, event_proc.name, event_proc.burst_est, queue);
                    }
                }
                else if(event_proc.status == 'i'){
                    (*(io+num)).status = 'w';
                    (*(waiting+index)).last_use = (*(waiting+index)).return_time;
                    if(event_proc.return_time <= 9999){
                        printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue %s\n", event_proc.return_time, event_proc.name, event_proc.burst_est, queue);
                    }
                    //remove from io
                    for(int j = num+1; j < num_io; j++){
                        (*(io+j-1)) = (*(io+j));
                    }
                    num_io--;
                }
                event_proc.status = 'w';
                
                if(num_waiting != 0){
                    free(queue);
                }
            }
        }

        //finish CPU Burst
        //get string for queue
        char* queue = get_queue(num_waiting, waiting);
        time += burst;
        //increment variables
        if(running.bursts_remaining > 1){
            running.return_time = time + *(running.io_bursts + (running.num_bursts - running.bursts_remaining)) + t_cs/2;
        }
        running.bursts_remaining--;
            
        if(running.bursts_remaining > 1){
            if(time <= 9999){ 
                printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d bursts to go %s\n", time, running.name, running.burst_est, running.bursts_remaining, queue);
            }
        }
        else if(running.bursts_remaining > 0){
            if(time <= 9999){ 
                printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d burst to go %s\n", time, running.name, running.burst_est, running.bursts_remaining, queue);
            }
        }
        running.last_use = time;
        //check if process terminated
        if(running.bursts_remaining == 0){
            printf("time %dms: Process %c terminated %s\n", time, running.name, queue);
            running.status = 'f';
            (*(finished+f_num)) = running;
            f_num++;
            
        }
        else{//io burst and burst_est
            //recalculate burst est
            int old_num = running.burst_est;
            running.burst_est = ceil(est*burst + (1.00 - est)*old_num);
            if(time <= 9999){ 
                printf("time %dms: Recalculating tau for process %c: old tau %dms ==> new tau %dms %s\n", time, running.name, old_num, running.burst_est, queue);
                printf("time %dms: Process %c switching out of CPU; blocking on I/O until time %dms %s\n", time, running.name, running.return_time, queue);
            }
            running.status = 'i';
            //add to io array
            *(io + num_io) = running;
            num_io++;
        }
        if(num_waiting != 0){
                free(queue);
        }
        if(f_num != len){
            //check if arrival or io end happened at same time as burst end
            //arrival
            event = true;
            event_time = -1;
            while(event){
                event_time = -1;
                event = false;
                int num = 0;
                //find arrival
                for(int i = 0; i < len; i++){
                    if(event_time != -1 && (*(unarrived+i)).status == 'u' && (*(unarrived+i)).return_time == event_time && (*(unarrived+i)).name < event_proc.name){
                    event = true;
                    event_time = (*(unarrived+i)).init_arriv;
                    event_proc = (*(unarrived+i));
                    num = i;
                }
                else if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+t_cs && ((unarrived+i)->init_arriv < event_time || event_time == -1)){
                    //printf("NAME: %c !!!!\n", (*(unarrived+i)).name);
                    event = true;
                    event_time = (*(unarrived+i)).init_arriv;
                    event_proc = (*(unarrived+i));
                    num = i;
                }
                }
                //find io finish
                for(int i = 0; i < num_io; i++){
                    if((*(io+i)).return_time < time+t_cs && ((*(io+i)).return_time < event_time || event_time == -1)){ 
                        event = true;
                        event_time = (*(io+i)).return_time;
                        event_proc = (*(io+i));
                        num = i;
                    }
                    else if((*(io+i)).return_time == event_time && event_proc.status == 'i' && (*(io+i)).name < event_proc.name){
                        event = true;
                        event_time = (*(io+i)).return_time;
                        event_proc = (*(io+i));
                        num = i;
                    }
                }
                if(event){
                    //move process to wait list
                    //add to waiting list
                    int index = num_waiting;
                    if(num_waiting != 0){
                        for(int j = 0; j < num_waiting; j++){
                            if(event_proc.burst_est < (*(waiting+j)).burst_est){
                                index = j;
                                j = num_waiting;
                            }
                            else if(event_proc.burst_est == (*(waiting+j)).burst_est && event_proc.name < (*(waiting+j)).name){
                                index = j;
                                j = num_waiting;
                            }
                        }
                        //num_waiting++;
                        if(index != num_waiting){
                            for(int j = num_waiting; j > index; j--){
                                //printf("NAME: %c!!!\n", (*(waiting+j)).name);
                                *(waiting+j) = *(waiting+j-1);
                            }
                            *(waiting+index) = event_proc;
                        }
                    }
                    num_waiting++;
                    *(waiting+index) = event_proc;
                    //printf("NAME: %c\n", event_proc.name);
                    //print messages
                    char* queue = get_queue(num_waiting, waiting);
                    if(((event_proc)).status == 'u'){
                        (*(unarrived+num)).status = 'w';
                        (*(waiting+index)).last_use = (*(waiting+index)).init_arriv;
                        if(event_proc.init_arriv <= 9999){ 
                            printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", event_proc.init_arriv, event_proc.name, event_proc.burst_est, queue);
                        }
                    }
                    else if(event_proc.status == 'i'){
                        (*(io+num)).status = 'w';
                        (*(waiting+index)).last_use = (*(waiting+index)).return_time;
                        if(event_proc.return_time <= 9999){ 
                            printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue %s\n", event_proc.return_time, event_proc.name, event_proc.burst_est, queue);
                        }
                        //remove from io
                        for(int j = num+1; j < num_io; j++){
                            (*(io+j-1)) = (*(io+j));
                        }
                        num_io--;
                    }
                    event_proc.status = 'w';
                    
                    if(num_waiting != 0){
                        free(queue);
                    }
                }
            }
            //change out running process
            if(num_waiting != 0){
                time += t_cs/2;
                running = *(waiting);
                //move up other waiting processes
                for(int i = 0; i < num_waiting - 1; i++){
                    *(waiting + i) = *(waiting + i + 1);
                }
                num_waiting--;
            }
            else{//if waiting queue is empty
                //check for soonest arriving io process
                process next;
                int least = -1;
                int index = 0;
                if(num_io > 0){
                    for(int i = 0; i < num_io; i++){
                        if((*(io+i)).return_time < least || least == -1){
                            next = *(io+i);
                            least = next.return_time;
                            index = i;
                        }
                    }
                }
                //check for unarrived processes
                for(int i = 0; i < len; i++){
                    if((*(unarrived+i)).status == 'u' && (*(unarrived+i)).init_arriv < least){
                        next = *(unarrived+i);
                        least = next.init_arriv;
                        index = i;
                    }
                }
                //arrive next process or return from io blocking to run
                running = next;
                if(running.status == 'i'){
                    running.last_use = running.return_time;
                    if(running.return_time <= 9999){ 
                        printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue [Q %c]\n", running.return_time, running.name, running.burst_est, running.name);
                    }
                    running.status = 'w';
                    time = running.return_time;
                    for(int j = index+1; j < num_io; j++){
                        (*(io+j-1)) = (*(io+j));
                    }
                    num_io--;
                }
                else{// status == u
                    running.last_use = running.init_arriv;
                    if(running.return_time <= 9999){ 
                        printf("time %dms: Process %c (tau %dms) arrived; added to ready queue [Q %c]\n", running.init_arriv, running.name, running.burst_est, running.name);
                    }
                    time = running.init_arriv;
                    (*(unarrived+index)).status = 'w';
                    running.status = 'w';
                }
            }
            //move process to be running
            running.status = 'r';
            queue = get_queue(num_waiting, waiting);
            //CHEKC MATH HERE!!!!!!!!!!!!!!!!!!!!!!!!
            time += t_cs/2;
            *(running.wait_time+(running.num_bursts-running.bursts_remaining)) = time - t_cs - running.last_use;
            *(running.turnaround_time+(running.num_bursts-running.bursts_remaining)) = *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining)) + t_cs/2 + (time - running.last_use);
            if(time <= 9999){     
                printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", time, running.name, running.burst_est, *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining)), queue);
            }
            if(num_waiting != 0){
                free(queue);
            }
            sys_burst += *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining));
        }
    }
    time += t_cs/2;
    printf("time %dms: Simulator ended for SJF [Q <empty>]\n\n", time);

    //file output
    FILE *fp;
    fp = fopen("simout.txt" ,"a");
    float use = sys_burst/time * 100;
    float tot_burst_num = 0;
    float tot_cpu_num = 0;
    float tot_io_num = 0;

    int cpu_cpu = 0;
    int io_cpu = 0;
    int tot_cpu = 0;

    int cpu_wait = 0;
    int io_wait = 0;
    int tot_wait = 0;

    int cpu_turn = 0;
    int io_turn = 0;
    int tot_turn;

    int tot_cs = 0;
    int cpu_cs = 0;
    int io_cs = 0;

    for(int i = 0; i < f_num; i++){
        tot_burst_num += (*(finished+i)).num_bursts;
        tot_cs += (*(finished+i)).num_bursts;
        if((*(finished+i)).type == 1){//cpu
            tot_cpu_num +=(*(finished+i)).num_bursts;
            cpu_cs += (*(finished+i)).num_bursts;
            for(int j = 0; j < (*(finished+i)).num_bursts; j++){
                cpu_cpu += *(((*(finished+i)).cpu_bursts)+j);
                cpu_wait += *(((*(finished+i)).wait_time)+j);
                cpu_turn += *(((*(finished+i)).turnaround_time)+j);
            }
        }
        else{//io
            tot_io_num +=(*(finished+i)).num_bursts;
            io_cs += (*(finished+i)).num_bursts;;
            for(int j = 0; j < (*(finished+i)).num_bursts; j++){
                io_cpu += *(((*(finished+i)).cpu_bursts)+j);
                io_wait += *(((*(finished+i)).wait_time)+j);
                
                io_turn += *(((*(finished+i)).turnaround_time)+j);
            }
        }
    }
    tot_cpu = cpu_cpu + io_cpu;
    tot_wait = cpu_wait + io_wait;
    tot_turn = cpu_turn + io_turn;

    fprintf(fp, "Algorithm SJF\n");
    fprintf(fp, "-- CPU utilization: %.3f%%\n", use);
    fprintf(fp, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_cpu/tot_burst_num*1000)/1000, ceil(cpu_cpu/tot_cpu_num*1000)/1000, ceil(io_cpu/tot_io_num*1000)/1000);
    fprintf(fp, "-- average wait time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_wait/tot_burst_num*1000)/1000, ceil(cpu_wait/tot_cpu_num*1000)/1000, ceil(io_wait/tot_io_num*1000)/1000);
    fprintf(fp, "-- average turnaround time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_turn/tot_burst_num*1000)/1000, ceil(cpu_turn/tot_cpu_num*1000)/1000, ceil(io_turn/tot_io_num*1000)/1000);
    fprintf(fp, "-- number of context switches: %d (%d/%d)\n", tot_cs, cpu_cs, io_cs);
    fprintf(fp, "-- number of preemptions: 0 (0/0)\n\n");
    //fputs("Algorithm SJF\n", fd);

    //free memory
    fclose(fp);
    free(io);
    free(waiting);
    free(finished);
}

void srt(process * unarrived, int len, int t_cs, float est, int upper){
    process running;
    float sys_burst = 0;
    running.name = 0;
    process * io = calloc(len, sizeof(process));
    process * waiting = calloc(len, sizeof(process));
    process * finished = calloc(len, sizeof(process));
    int f_num = 0;
    int num_waiting = 0;
    int num_io = 0;
    int time = 0;
    int first = -1;
    int io_preempt = 0;
    int cpu_preempt = 0;
    int io_cs = 0;
    int cpu_cs = 0;

    //get burst times for file output
    int cpu_cpu = 0;
    int io_cpu = 0;
    for(int i = 0; i < len; i++){
        for(int j = 0; j < (*(unarrived+i)).num_bursts; j++){
            if((*(unarrived+i)).type == 1){//cpu
                cpu_cpu += (*((*(unarrived+i)).cpu_bursts+j));
            }
            else{
                io_cpu += (*((*(unarrived+i)).cpu_bursts+j));
            }
        }
    }

    printf("time 0ms: Simulator started for SRT [Q <empty>]\n");
    //find earliest arrivel
    for(int i = 0; i < len; i++){
        if(((unarrived+i)->init_arriv < first || first == -1) && (unarrived+i)->status == 'u'){
            first = (unarrived+i)->init_arriv;
        }
    }
    time = first;

    //get initial waiting processes
    for(int i = 0; i < len; i++){
        if((unarrived+i)->init_arriv == first && (unarrived+i)->status == 'u'){
           *(waiting + num_waiting) = *(unarrived+i);
           num_waiting ++;
            //queue string
           char* queue = get_queue(num_waiting, waiting);
            printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", time, (*(unarrived+i)).name, (*(unarrived+i)).burst_est, queue);
            (unarrived+i)->status = 'w';
            if(num_waiting != 0){
                free(queue);
            }
            (*(waiting +num_waiting)).last_use = time;
        }
    }

    //get first process
    first = -1;
    for(int i = 0; i < num_waiting; i++){
        if((waiting+i)->burst_est < first || first == -1){
            running = *(waiting+i);
        }
    }
    num_waiting--;
    time += t_cs/2;

    //need to check for other arrivals 
    char* queue = get_queue(num_waiting, waiting);
    if(running.type == 1){//cpu
        cpu_cs++;
    }
    else{//io
        io_cs++;
    }
    printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", time, running.name, running.burst_est, *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining)), queue);
    sys_burst += *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining));
    *(running.turnaround_time+(running.num_bursts-running.bursts_remaining)) = *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining)) + t_cs/2 + (time - running.init_arriv);
    *(running.wait_time+(running.num_bursts-running.bursts_remaining)) = time - t_cs/2 - running.last_use;
    if(num_waiting != 0){
        free(queue);
    }
    running.status = 'r';
    int burst = *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining));
    //srt algorithm
    while(f_num < len){
        /*int burst;
        if(running.preed){
            burst = running.prev;
        }
        else{
            burst = *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining));
        }*/
        bool event = true;
        int event_time = -1;
        process event_proc;
        //int preempt_switch = -1;
        bool cs = false;
        bool half_cs = false;
        //int cs_time = -1;
        process preempted;
        //find newest arrivals or io if current CPU burst exceeds arrival time
        //printf("TIME: %d \n", time+burst);
        while(event){
            event_time = -1;
            event = false;
            int num = 0;
            //find arrival
            for(int i = 0; i < len; i++){
                //printf("NAME: %c !!!! %d %c\n", (*(unarrived+i)).name, (*(unarrived+i)).init_arriv, (*(unarrived+i)).status);
                if(event_time != -1 && (*(unarrived+i)).status == 'u' && (*(unarrived+i)).return_time == event_time && (*(unarrived+i)).name < event_proc.name){
                    event = true;
                    event_time = (*(unarrived+i)).init_arriv;
                    event_proc = (*(unarrived+i));
                    num = i;
                }
                else if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+burst && ((unarrived+i)->init_arriv < event_time || event_time == -1)){
                    //printf("NAME: %c !!!!\n", (*(unarrived+i)).name);
                    event = true;
                    event_time = (*(unarrived+i)).init_arriv;
                    event_proc = (*(unarrived+i));
                    num = i;
                }
                
            }
            //find io finish
            for(int i = 0; i < num_io; i++){
                if((*(io+i)).return_time < time+burst && ((*(io+i)).return_time < event_time || event_time == -1)){ 
                    event = true;
                    event_time = (*(io+i)).return_time;
                    event_proc = (*(io+i));
                    num = i;
                }
                else if((*(io+i)).return_time == event_time && event_proc.status == 'i' && (*(io+i)).name < event_proc.name){
                    event = true;
                    event_time = (*(io+i)).return_time;
                    event_proc = (*(io+i));
                    num = i;
                }
            }
            if(event){
                //check for preemption
                if(event_proc.status != 'u' && event_proc.burst_est < (time+running.burst_est)-event_time  && (!running.preed || (running.preed && event_proc.burst_est < (time+running.burst_est) - (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))-running.prev) - event_time))){
                //((!running.preed && event_proc.burst_est < (time+running.burst_est)-event_time) || (running.preed && event_proc.burst_est < running.prev))){
                    //switch event_proc to running
                    //preempt_switch = event_time;
                    preempted = running;
                    if(event_time != time){
                        preempted.preed = true;
                    }
                    //update cpu_burst time
                    preempted.prev = (burst-(event_time-time));
                    //increment preempt
                    if(preempted.type == 1){//cpu
                        cpu_preempt++;
                    }
                    else{//io
                        io_preempt++;
                    }
                    
                    running = event_proc;
                    //add to front of waiting queue
                    for(int j = num_waiting; j > 0; j--){
                        //printf("NAME: %c!!!\n", (*(waiting+j)).name);
                        *(waiting+j) = *(waiting+j-1);
                    }
                    num_waiting++;
                    *(waiting) = event_proc;
                    //print statement

                    time = event_time;
                    queue = get_queue(num_waiting, waiting);
                    if(time <= 9999){ 
                        printf("time %dms: Process %c (tau %dms) completed I/O; preempting %c %s\n", time, running.name, running.burst_est, preempted.name, queue);
                    }
                    if(num_waiting != 0){
                        free(queue);
                    }
                    //remove runing from waiting
                    running.status = 'r';
                    num_waiting--;
                    for(int j = 0; j < num_waiting; j++){
                        (*(waiting+j)) = (*(waiting+j+1));
                    }
                    //fix status
                    if(((event_proc)).status == 'u'){
                        (*(unarrived+num)).status = 'w';
                        
                    }
                    else if(event_proc.status == 'i'){
                        (*(io+num)).status = 'w';
                        //remove from io
                        for(int j = num+1; j < num_io; j++){
                            (*(io+j-1)) = (*(io+j));
                        }
                        num_io--;
                    }
                    //check for occurrences during cs
                    cs = false;
                    half_cs = false;
                    //cs_time = -1;
                    for(int i = 0; i < len; i++){
                        if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+t_cs/2){ 
                            //cs_time = (*(io+i)).return_time;
                            half_cs = true;
                        }
                        if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+t_cs){
                            //cs_time = (*(unarrived+i)).init_arriv;
                            cs = true;
                        }
                    }
                    for(int i = 0; i < num_io; i++){
                        if((*(io+i)).return_time < time+t_cs/2){ 
                            //cs_time = (*(io+i)).return_time;
                            half_cs = true;
                        }
                        if((*(io+i)).return_time < time+t_cs){ 
                            //cs_time = (*(io+i)).return_time;
                            cs = true;
                        }
                    }
                    if(!half_cs){
                        //add preempted to waiting
                        //if(index != num_waiting){
                        for(int j = num_waiting; j > 0; j--){
                            //printf("NAME: %c!!!\n", (*(waiting+j)).name);
                            *(waiting+j) = *(waiting+j-1);
                        }
                        *(waiting) = preempted;
                        num_waiting++;
                        half_cs = false;
                    }
                    if(!cs){
                        //cpu burst print statement
                        time += t_cs;
                        queue = get_queue(num_waiting, waiting);
                        if(running.type == 1){//cpu
                            cpu_cs++;
                        }
                        else{//io
                            io_cs++;
                        }
                        if(!running.preed){
                            burst = (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining)));
                            if(time <= 9999){ 
                                printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", time, running.name, running.burst_est, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                            }
                        }
                        else{
                            burst = running.prev;
                            //printf("WHATTTTT: %d\n", burst);
                            if(time <= 9999){ 
                                printf("time %dms: Process %c (tau %dms) started using the CPU for remaining %dms of %dms burst %s\n", time, running.name, running.burst_est, running.prev, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                            }
                        }
                        if(num_waiting != 0){
                            free(queue);
                        }
                        cs = false;
                        
                    }
                    if(!running.preed){
                        burst = (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining)));
                     }
                    else{
                        burst = running.prev;
                    }
                }
                else{
                    if(half_cs && time+t_cs/2 < event_time){// add preempted to waiting
                        //if(index != num_waiting){
                        for(int j = num_waiting; j > 0; j--){
                            //printf("NAME: %c!!!\n", (*(waiting+j)).name);
                            *(waiting+j) = *(waiting+j-1);
                        }
                        *(waiting) = preempted;
                        num_waiting++;
                        half_cs = false;
                    }
                    if(cs && time+t_cs < event_time){//start cpu burst runnning
                        //cpu burst print statement
                        time += t_cs;
                        queue = get_queue(num_waiting, waiting);
                        if(running.type == 1){//cpu
                            cpu_cs++;
                        }
                        else{//io
                            io_cs++;
                        }
                        if(!running.preed){
                            burst = (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining)));
                            if(time <= 9999){ 
                                printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", time, running.name, running.burst_est, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                            }
                        }
                        else{
                            burst = running.prev;
                            //printf("WHATTTTT: %d\n", burst);
                            if(time <= 9999){ 
                                printf("time %dms: Process %c (tau %dms) started using the CPU for remaining %dms of %dms burst %s\n", time, running.name, running.burst_est, running.prev, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                            }
                            //running.preed = false;
                        }
                        if(num_waiting != 0){
                            free(queue);
                        }
                        cs = false;
                    }
                    //add to waiting list
                    int index = num_waiting;
                    if(num_waiting != 0){
                        for(int j = 0; j < num_waiting; j++){
                            
                            if((event_proc.burst_est < (*(waiting+j)).burst_est && !(*(waiting+j)).preed)|| ((*(waiting+j)).preed && event_proc.burst_est < (*(waiting+j)).burst_est-((*((*(waiting+j)).cpu_bursts+(*(waiting+j)).num_bursts-(*(waiting+j)).bursts_remaining))-(*(waiting+j)).prev))){
                                index = j;
                                j = num_waiting;
                            }
                            else if(event_proc.burst_est == (*(waiting+j)).burst_est && event_proc.name < (*(waiting+j)).name){
                                index = j;
                                j = num_waiting;
                            }
                        }
                        //num_waiting++;
                        if(index != num_waiting){
                            for(int j = num_waiting; j > index; j--){
                                //printf("NAME: %c!!!\n", (*(waiting+j)).name);
                                *(waiting+j) = *(waiting+j-1);
                            }
                            *(waiting+index) = event_proc;
                        }
                    }
                    num_waiting++;
                    *(waiting+index) = event_proc;
                    //check if next event happens during preemption context switch

                    //print messages
                    char* queue = get_queue(num_waiting, waiting);
                    if(((event_proc)).status == 'u'){
                        (*(unarrived+num)).status = 'w';
                        (*(waiting+index)).last_use = (*(waiting+index)).init_arriv;
                        if(event_proc.init_arriv <= 9999){ 
                            printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", event_proc.init_arriv, event_proc.name, event_proc.burst_est, queue);
                        }
                    }
                    else if(event_proc.status == 'i'){
                        (*(io+num)).status = 'w';
                        (*(waiting+index)).last_use = (*(waiting+index)).return_time;
                        if(event_proc.return_time <= 9999){ 
                            printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue %s\n", event_proc.return_time, event_proc.name, event_proc.burst_est, queue);
                        }
                        //remove from io
                        for(int j = num+1; j < num_io; j++){
                            (*(io+j-1)) = (*(io+j));
                        }
                        num_io--;
                    }
                    event_proc.status = 'w';
                    
                    if(num_waiting != 0){
                        free(queue);
                    }
                }
            }
        }

        //ifcs switch interruption is only event that occurs
        if(cs ){// add preempted to waiting
            if(half_cs){//if(index != num_waiting){
                for(int j = num_waiting; j > 0; j--){
                    //printf("NAME: %c!!!\n", (*(waiting+j)).name);
                    *(waiting+j) = *(waiting+j-1);
                }
                *(waiting) = preempted;
                num_waiting++;
                half_cs = false;
            }
            //cs = false;
            //start cpu burst runnning
            //cpu burst print statement
            time += t_cs;
            queue = get_queue(num_waiting, waiting);
            if(running.type == 1){//cpu
                cpu_cs++;
            }
            else{//io
                io_cs++;
            }
            if(!running.preed){
                burst = (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining)));
                if(time <= 9999){ 
                    printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", time, running.name, running.burst_est, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                }
            }
            else{
                burst = running.prev;
                //printf("WHATTTTT: %d\n", burst);
                if(time <= 9999){ 
                    printf("time %dms: Process %c (tau %dms) started using the CPU for remaining %dms of %dms burst %s\n", time, running.name, running.burst_est, running.prev, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                }
                //running.preed = false;
            }
            if(num_waiting != 0){
                free(queue);
            }
            cs = false;
            half_cs = false;
        }

        //finish CPU Burst
        //get string for queue
        char* queue = get_queue(num_waiting, waiting);
        //printf("HEYYYYY: %d\n", burst);
        time += burst;
        //increment variables
        burst = (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining)));
        if(running.bursts_remaining > 1){
            running.return_time = time + *(running.io_bursts + (running.num_bursts - running.bursts_remaining)) + t_cs/2;
        }
        running.bursts_remaining--;
        
        running.preed = false;
        if(running.bursts_remaining > 1){
            if(time <= 9999){ 
                printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d bursts to go %s\n", time, running.name, running.burst_est, running.bursts_remaining, queue);
            }
        }
        else if(running.bursts_remaining > 0){
            if(time <= 9999){ 
                printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d burst to go %s\n", time, running.name, running.burst_est, running.bursts_remaining, queue);
            }
        }
        running.last_use = time;
        //check if process terminated
        if(running.bursts_remaining == 0){
            printf("time %dms: Process %c terminated %s\n", time, running.name, queue);
            running.status = 'f';
            (*(finished+f_num)) = running;
            f_num++;
            
        }
        else{//io burst and burst_est
            //recalculate burst est
            int old_num = running.burst_est;
            running.burst_est = ceil(est*burst + (1.00 - est)*old_num);
            if(time <= 9999){ 
                printf("time %dms: Recalculating tau for process %c: old tau %dms ==> new tau %dms %s\n", time, running.name, old_num, running.burst_est, queue);
                printf("time %dms: Process %c switching out of CPU; blocking on I/O until time %dms %s\n", time, running.name, running.return_time, queue);
            }
            running.status = 'i';
            //add to io array
            *(io + num_io) = running;
            num_io++;
        }
        if(num_waiting != 0){
            free(queue);
        }
        if(f_num != len){
            //check if arrival or io end happened at same time as burst end
            
            //arrival
            event = true;
            event_time = -1;
            bool cs = false;
            bool preed = false;
            while(event){
                event_time = -1;
                event = false;
                int num = 0;
                //find arrival
                for(int i = 0; i < len; i++){
                    if((*(unarrived+i)).status == 'u' && (*(unarrived+i)).init_arriv == time && (*(unarrived+i)).name < event_proc.name){
                        event = true;
                        event_time = (*(unarrived+i)).init_arriv;
                        event_proc = (*(unarrived+i));
                        num = i;
                    }
                    else if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+t_cs && ((unarrived+i)->init_arriv <= event_time || event_time == -1)){
                        //printf("NAME: %c !!!!\n", (*(unarrived+i)).name);
                        event = true;
                        event_time = (*(unarrived+i)).init_arriv;
                        event_proc = (*(unarrived+i));
                        num = i;
                        cs = true;
                    }
                    else if((*(unarrived+i)).status =='u' && (*(unarrived+i)).init_arriv == event_time && (*(unarrived+i)).name < event_proc.name){
                        event = true;
                        event_time = (*(unarrived+i)).init_arriv;
                        event_proc = (*(unarrived+i));
                        num = i;
                    }
                }
                //find io finish
                for(int i = 0; i < num_io; i++){
                    if((*(io+i)).return_time == time || (*(io+i)).return_time <= time+ t_cs/2){
                        event = true;
                        event_time = (*(io+i)).return_time;
                        event_proc = (*(io+i));
                        num = i;
                    }
                    else if((*(io+i)).return_time < time+t_cs && ((*(io+i)).return_time < event_time || event_time == -1)){ 
                        event = true;
                        event_time = (*(io+i)).return_time;
                        event_proc = (*(io+i));
                        num = i;
                        cs = true;
                    }
                    if((*(io+i)).return_time == event_time && event_proc.status == 'i' && (*(io+i)).name < event_proc.name){
                        event = true;
                        event_time = (*(io+i)).return_time;
                        event_proc = (*(io+i));
                        num = i;
                    }
                }
                if(event){
                    //event occuers in context switch
                    if(cs){
                        time += t_cs/2;
                        running = *(waiting);
                        //move up other waiting processes
                        for(int i = 0; i < num_waiting - 1; i++){
                            *(waiting + i) = *(waiting + i + 1);
                        }
                        num_waiting--;
                    }
                    //move process to wait list
                    //add to waiting list
                    int index = num_waiting;
                    if(num_waiting != 0){
                        for(int j = 0; j < num_waiting; j++){
                            if((event_proc.burst_est < (*(waiting+j)).burst_est && !(*(waiting+j)).preed)|| ((*(waiting+j)).preed && event_proc.burst_est < (*(waiting+j)).burst_est-((*((*(waiting+j)).cpu_bursts+(*(waiting+j)).num_bursts-(*(waiting+j)).bursts_remaining))-(*(waiting+j)).prev))){
                                index = j;
                                j = num_waiting;
                            }
                            else if(event_proc.burst_est == (*(waiting+j)).burst_est && event_proc.name < (*(waiting+j)).name){
                                index = j;
                                j = num_waiting;
                            }
                        }
                        //num_waiting++;
                        if(index != num_waiting){
                            for(int j = num_waiting; j > index; j--){
                                *(waiting+j) = *(waiting+j-1);
                            }
                            *(waiting+index) = event_proc;
                        }
                    }
                    num_waiting++;
                    *(waiting+index) = event_proc;
                    //check for wait preemption
                    //print messages
                    char* queue = get_queue(num_waiting, waiting);
                    if(((event_proc)).status == 'u'){
                        (*(unarrived+num)).status = 'w';
                        (*(waiting+index)).last_use = (*(waiting+index)).init_arriv;
                        if(event_proc.init_arriv <= 9999){ 
                            printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", event_proc.init_arriv, event_proc.name, event_proc.burst_est, queue);
                        }
                    }
                    else if(event_proc.status == 'i'){
                        (*(io+num)).status = 'w';
                        (*(waiting+index)).last_use = (*(waiting+index)).return_time;
                        if(event_proc.return_time <= 9999){
                            printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue %s\n", event_proc.return_time, event_proc.name, event_proc.burst_est, queue);
                        }
                        //remove from io
                        for(int j = num+1; j < num_io; j++){
                            (*(io+j-1)) = (*(io+j));
                        }
                        num_io--;
                    }
                    event_proc.status = 'w';
                    if(event_proc.status != 'u' && index == 0 && event_proc.burst_est < (time+running.burst_est)-event_time  && (!running.preed || (running.preed && event_proc.burst_est < (time+running.burst_est) - (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))-running.prev) - event_time))){
                        preed = true;
                    }
                    if(num_waiting != 0){
                        free(queue);
                    }
                }
            }
            //change out running process
            if(num_waiting != 0){
                if(!cs){
                    time += t_cs/2;
                    running = *(waiting);
                    //move up other waiting processes
                    for(int i = 0; i < num_waiting - 1; i++){
                        *(waiting + i) = *(waiting + i + 1);
                    }
                    num_waiting--;
                }
                else{
                    cs = false;
                }
            }
            else{//if waiting queue is empty
                //check for soonest arriving io process
                process next;
                int least = -1;
                int index = 0;
                if(num_io > 0){
                    for(int i = 0; i < num_io; i++){
                        if((*(io+i)).return_time < least || least == -1){
                            next = *(io+i);
                            least = next.return_time;
                            index = i;
                        }
                    }
                }
                //check for unarrived processes
                for(int i = 0; i < len; i++){
                    if((*(unarrived+i)).status == 'u' && (*(unarrived+i)).init_arriv < least){
                        next = *(unarrived+i);
                        least = next.init_arriv;
                        index = i;
                    }
                }
                //arrive next process or return from io blocking to run
                running = next;
                if(running.status == 'i'){
                    running.last_use = running.return_time;
                    if(running.return_time <= 9999){ 
                        printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue [Q %c]\n", running.return_time, running.name, running.burst_est, running.name);
                    }
                    running.status = 'w';
                    time = running.return_time;
                    for(int j = index+1; j < num_io; j++){
                        (*(io+j-1)) = (*(io+j));
                    }
                    num_io--;
                }
                else{// status == u
                    running.last_use = running.init_arriv;
                    if(running.init_arriv <= 9999){ 
                        printf("time %dms: Process %c (tau %dms) arrived; added to ready queue [Q %c]\n", running.init_arriv, running.name, running.burst_est, running.name);
                    }
                    time = running.init_arriv;
                    (*(unarrived+index)).status = 'w';
                    running.status = 'w';
                }
            }
            //move process to be running

            running.status = 'r';
            queue = get_queue(num_waiting, waiting);
            time += t_cs/2;
            *(running.wait_time+(running.num_bursts-running.bursts_remaining)) = time - t_cs/2 - running.last_use;
            *(running.turnaround_time+(running.num_bursts-running.bursts_remaining)) = *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining)) + t_cs/2 + (time - running.last_use);
            if(running.type == 1){//cpu
                cpu_cs++;
            }
            else{//io
                io_cs++;
            }
            if(!running.preed){
                if(time <= 9999){ 
                    printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", time, running.name, running.burst_est, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                }
                burst = (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining)));
            }
            else{
                if(time <= 9999){ 
                    printf("time %dms: Process %c (tau %dms) started using the CPU for remaining %dms of %dms burst %s\n", time, running.name, running.burst_est, running.prev, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                }
                burst = running.prev;
                //running.preed = false;
            }
            if(preed){
                if(running.type == 1){
                    cpu_preempt++;
                }
                else{
                    io_preempt++;
                }
                if(time <= 9999){ 
                    printf("time %dms: Process %c (tau %dms) will preempt %c %s\n", time, (*waiting).name, (*waiting).burst_est, running.name, queue);
                }
                if(num_waiting != 0){
                    free(queue);
                } 
                //switch out first position of wait queue and start new process
                process new_proc = (*waiting);
                (*waiting) = running;
                running = new_proc;
                queue = get_queue(num_waiting, waiting);
                time += t_cs;
                if(time <= 9999){ 
                    printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", time, running.name, running.burst_est, (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining))), queue);
                }
                *(running.wait_time+(running.num_bursts-running.bursts_remaining)) = time - t_cs/2 - running.last_use;
                *(running.turnaround_time+(running.num_bursts-running.bursts_remaining)) = *(running.cpu_bursts + (running.num_bursts-running.bursts_remaining)) + t_cs/2 + (time - running.last_use);
                burst = (*(running.cpu_bursts+(running.num_bursts-running.bursts_remaining)));
                if(running.type == 1){//cpu
                    cpu_cs++;
                }
                else{//io
                    io_cs++;
                }
                }
                if(num_waiting != 0){
                    free(queue);
                } 
            
        }
    }
    time += t_cs/2;
    printf("time %dms: Simulator ended for SRT [Q <empty>]\n\n", time);

    //file output
    float tot_burst_num = 0;
    float tot_cpu_num = 0;
    float tot_io_num = 0;
    float tot_cpu = io_cpu + cpu_cpu;
    int cpu_wait = 0;
    int io_wait = 0;
    int tot_wait = 0;
    int cpu_turn = 0;
    int io_turn = 0;
    int tot_turn;
    int tot_cs = cpu_cs+io_cs;

    for(int i = 0; i < f_num; i++){
        tot_burst_num += (*(finished+i)).num_bursts;
        if((*(finished+i)).type == 1){//cpu
            tot_cpu_num +=(*(finished+i)).num_bursts;
            for(int j = 0; j < (*(finished+i)).num_bursts; j++){
                cpu_wait += *(((*(finished+i)).wait_time)+j);
                cpu_turn += *(((*(finished+i)).turnaround_time)+j);
            }
        }
        else{//io
            tot_io_num +=(*(finished+i)).num_bursts;
            for(int j = 0; j < (*(finished+i)).num_bursts; j++){
                io_wait += *(((*(finished+i)).wait_time)+j);
                
                io_turn += *(((*(finished+i)).turnaround_time)+j);
            }
        }
    }
    tot_cpu = cpu_cpu + io_cpu;
    tot_wait = cpu_wait + io_wait;
    tot_turn = cpu_turn + io_turn;
    float use = tot_cpu/time * 100;
    FILE *fp;
    fp = fopen("simout.txt" ,"a");

    fprintf(fp, "Algorithm SRT\n");
    fprintf(fp, "-- CPU utilization: %.3f%%\n", ceil(use*1000)/1000);
    fprintf(fp, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_cpu/tot_burst_num*1000)/1000, ceil(cpu_cpu/tot_cpu_num*1000)/1000, ceil(io_cpu/tot_io_num*1000)/1000);
    fprintf(fp, "-- average wait time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_wait/tot_burst_num*1000)/1000, ceil(cpu_wait/tot_cpu_num*1000)/1000, ceil(io_wait/tot_io_num*1000)/1000);
    fprintf(fp, "-- average turnaround time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_turn/tot_burst_num*1000)/1000, ceil(cpu_turn/tot_cpu_num*1000)/1000, ceil(io_turn/tot_io_num*1000)/1000);
    fprintf(fp, "-- number of context switches: %d (%d/%d)\n", tot_cs, cpu_cs, io_cs);
    fprintf(fp, "-- number of preemptions: %d (%d/%d)\n\n", cpu_preempt+io_preempt, cpu_preempt, io_preempt);
    //fputs("Algorithm SJF\n", fd);}

    //free memory
    fclose(fp);
    free(io);
    free(waiting);
    free(finished);
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
    float alpha =  atof(*(argv+7));
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
        ptr->bursts_remaining = bursts;

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
        ptr->burst_est = floor(1.000/interarrival);
        ptr->status = 'u';
        ptr->wait_time = calloc(ptr->num_bursts, sizeof(int));
        ptr->turnaround_time = calloc(ptr->num_bursts, sizeof(int));
        ptr->preempt = 0;
        ptr->preed = false;
        ptr->prev = 0;
        ptr->fut_pre = false;
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

    process * algo_copy = calloc(num_proc, sizeof(process));
    process * algo_copy2 = calloc(num_proc, sizeof(process));

    for(int i = 0; i < num_proc; i++){
        *(algo_copy+i) = *(processes+i);
        *(algo_copy2+i) = *(processes+i);
    }



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

    /*process* processesSJF = calloc(num_proc, sizeof(process));
    
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        process* ptr2 = processesSJF + i;
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
        ptr2->burst_est = ceil(1.00/interarrival);
        ptr2->status = 'u';
        ptr2->wait_time = calloc(ptr->num_bursts, sizeof(int));
        ptr2->turnaround_time = calloc(ptr->num_bursts, sizeof(int));
        ptr2->preempt = 0;
        ptr2->preed = false;
        ptr2->prev = 0;
        ptr2->fut_pre = false;
    }
    
    process* processesSRT = calloc(num_proc, sizeof(process));
    
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        process* ptr2 = processesSRT + i;
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
        ptr2->burst_est = ceil(1.00/interarrival);
        ptr2->status = 'u';
        ptr2->wait_time = calloc(ptr->num_bursts, sizeof(int));
        ptr2->turnaround_time = calloc(ptr->num_bursts, sizeof(int));
        ptr2->preempt = 0;
        ptr2->preed = false;
        ptr2->prev = 0;
        ptr2->fut_pre = false;
    }*/

    printf("\n<<< PROJECT PART II -- t_cs=%dms; alpha=%.2f; t_slice=%dms >>>\n", time_cs, alpha, t_slice);

    FCFS(processesFCFS, num_proc, time_cs/2);

    sjf(algo_copy, num_proc, time_cs, alpha, upper_bound);

    srt(algo_copy2, num_proc, time_cs, alpha, upper_bound);

    RR(processesRR, num_proc, time_cs/2, t_slice);
    

    for(int i=0; i<num_proc; i++){
        process* ptr = processesFCFS + i;
        free(ptr->io_bursts);
        free(ptr->cpu_bursts);
        free(ptr->wait_time);
        free(ptr->turnaround_time);
    }
    free(processesFCFS);

    for(int i=0; i<num_proc; i++){
        process* ptr = processesRR + i;
        free(ptr->io_bursts);
        free(ptr->cpu_bursts);
        free(ptr->wait_time);
        free(ptr->turnaround_time);
    }
    free(processesRR);

    /*for(int i=0; i<num_proc; i++){
        process* ptr = processesSJF + i;
        free(ptr->io_bursts);
        free(ptr->cpu_bursts);
        free(ptr->wait_time);
        free(ptr->turnaround_time);
    }*/
    free(algo_copy);

    /*for(int i=0; i<num_proc; i++){
        process* ptr = processesSRT + i;
        free(ptr->io_bursts);
        free(ptr->cpu_bursts);
        free(ptr->wait_time);
        free(ptr->turnaround_time);
    }*/
    free(algo_copy2);

    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        free(ptr->cpu_bursts);
        free(ptr->io_bursts);
        free(ptr->wait_time);
        free(ptr->turnaround_time);
    }
    free(processes);


}