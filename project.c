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

//First Come First Serve (FCFS) Algorithm

void fcfs(){}

//Shortest Job First (SJF) Algorithm
void sjf(process * unarrived, int len, int t_cs, float est, int upper, FILE * fp){
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
                //printf("NAME: %c !!!! %d %c\n", (*(unarrived+i)).name, (*(unarrived+i)).init_arriv, (*(unarrived+i)).status);
                if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+burst && ((unarrived+i)->init_arriv <= event_time || event_time == -1)){
                    //printf("NAME: %c !!!!\n", (*(unarrived+i)).name);
                    event = true;
                    event_time = (*(unarrived+i)).init_arriv;
                    event_proc = (*(unarrived+i));
                    num = i;
                }
                else if((*(unarrived+i)).return_time == event_time && (*(unarrived+i)).name < event_proc.name){
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
                    //printf("NAME: %c !!!! %d %c\n", (*(unarrived+i)).name, (*(unarrived+i)).init_arriv, (*(unarrived+i)).status);
                    if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+t_cs && ((unarrived+i)->init_arriv <= event_time || event_time == -1)){
                        //printf("NAME: %c !!!!\n", (*(unarrived+i)).name);
                        event = true;
                        event_time = (*(unarrived+i)).init_arriv;
                        event_proc = (*(unarrived+i));
                        num = i;
                    }
                    else if((*(unarrived+i)).return_time == event_time && (*(unarrived+i)).name < event_proc.name){
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
    fprintf(fp, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_cpu/tot_burst_num*1000.0)/1000, ceil(cpu_cpu/tot_cpu_num*1000.0)/1000, ceil(io_cpu/tot_io_num*1000.0)/1000);
    fprintf(fp, "-- average wait time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_wait/tot_burst_num*1000.0)/1000, ceil(cpu_wait/tot_cpu_num*1000.0)/1000, ceil(io_wait/tot_io_num*1000.0)/1000);
    fprintf(fp, "-- average turnaround time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_turn/tot_burst_num*1000.0)/1000, ceil(cpu_turn/tot_cpu_num*1000.0)/1000, ceil(io_turn/tot_io_num*1000.0)/1000);
    fprintf(fp, "-- number of context switches: %d (%d/%d)\n", tot_cs, cpu_cs, io_cs);
    fprintf(fp, "-- number of preemptions: 0 (0/0)\n\n");
    //fputs("Algorithm SJF\n", fd);

    //free memory
    free(io);
    free(waiting);
    free(finished);
}

//Shortest Remaining Time (SRT) Algorithm

void srt(process * unarrived, int len, int t_cs, float est, int upper, FILE * fp){
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
                if((*(unarrived+i)).status == 'u' && (unarrived+i)->init_arriv < time+burst && ((unarrived+i)->init_arriv <= event_time || event_time == -1)){
                    //printf("NAME: %c !!!!\n", (*(unarrived+i)).name);
                    event = true;
                    event_time = (*(unarrived+i)).init_arriv;
                    event_proc = (*(unarrived+i));
                    num = i;
                }
                else if((*(unarrived+i)).return_time == event_time && (*(unarrived+i)).name < event_proc.name){
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

    fprintf(fp, "Algorithm SRT\n");
    fprintf(fp, "-- CPU utilization: %.3f%%\n", ceil(use*1000.0)/1000);
    fprintf(fp, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_cpu/tot_burst_num*1000.0)/1000, ceil(cpu_cpu/tot_cpu_num*1000.0)/1000, ceil(io_cpu/tot_io_num*1000.0)/1000);
    fprintf(fp, "-- average wait time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_wait/tot_burst_num*1000.0)/1000, ceil(cpu_wait/tot_cpu_num*1000.0)/1000, ceil(io_wait/tot_io_num*1000.0)/1000);
    fprintf(fp, "-- average turnaround time: %.3f ms (%.3f ms/%.3f ms)\n", ceil(tot_turn/tot_burst_num*1000.0)/1000, ceil(cpu_turn/tot_cpu_num*1000.0)/1000, ceil(io_turn/tot_io_num*1000.0)/1000);
    fprintf(fp, "-- number of context switches: %d (%d/%d)\n", tot_cs, cpu_cs, io_cs);
    fprintf(fp, "-- number of preemptions: %d (%d/%d)\n\n", cpu_preempt+io_preempt, cpu_preempt, io_preempt);
    //fputs("Algorithm SJF\n", fd);}

    //free memory
    free(io);
    free(waiting);
    free(finished);
}
//Round Robin (RR) Algorithm

void rr(){}

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

    if(atoi(*(argv+6)) < 0 || isalpha(**(argv+6)) || atoi(*(argv+6))%2 != 0){
        fprintf(stderr, "ERROR: Invalid number of Context Switch Time\n");
        return EXIT_FAILURE;
    }

    if(atoi(*(argv+7)) < 0 || isalpha(**(argv+7))){
        fprintf(stderr, "ERROR: Invalid value for CPU Burst Time estimate\n");
        return EXIT_FAILURE;
    }

    if(atoi(*(argv+8)) < 0 || isalpha(**(argv+8))){
        fprintf(stderr, "ERROR: Invalid value for Time Slice\n");
        return EXIT_FAILURE;
    }

    //assign input variables
    int num_proc = atoi(*(argv+1));
    int cpu_bound = atoi(*(argv+2));
    int seed = atoi(*(argv+3));
    float interarrival = atof(*(argv+4));
    int upper_bound = atoi(*(argv+5));
    int context_switch = atoi(*(argv+6));
    float cpu_est = atof(*(argv+7));
    //int time_slice = atoi(*(argv+8));

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
        //cpu est
        ptr->burst_est = ceil(1.00/interarrival);
        ptr->status = 'u';
        ptr->wait_time = calloc(ptr->num_bursts, sizeof(int));
        ptr->turnaround_time = calloc(ptr->num_bursts, sizeof(int));
        ptr->preempt = 0;
        ptr->preed = false;
        ptr->prev = 0;
        ptr->fut_pre = false;
    }

    //PART 1 OUTPUT
    printf("<<< PROJECT PART I -- process set (n=%d) with %d CPU-bound process >>>\n", num_proc, cpu_bound);
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        if(ptr->type == 1){
            printf("CPU");
        }
        else{
            printf("I/O");
        }
        printf("-bound process %c: arrival time %dms; %d CPU bursts\n", ptr->name, ptr->init_arriv, ptr->num_bursts);
    }
    printf("\n");
    //open new file with write permissions and pass fd to algorithm
    FILE * fp = fopen("simout.txt", "w");

    //fun sjf algorithm
    process * algo_copy = calloc(num_proc, sizeof(process));

    for(int i = 0; i < num_proc; i++){
        *(algo_copy+i) = *(processes+i);
    }
    sjf(algo_copy, num_proc, context_switch, cpu_est, upper_bound, fp);

    
    for(int i = 0; i < num_proc; i++){
        *(algo_copy+i) = *(processes+i);
    }
    srt(algo_copy, num_proc, context_switch, cpu_est, upper_bound, fp);

    fclose(fp);
    //free memory
    free(algo_copy);
    for(int i = 0; i < num_proc; i++){
        process* ptr = processes + i;
        free(ptr->cpu_bursts);
        free(ptr->io_bursts);
        free(ptr->wait_time);
        free(ptr->turnaround_time);
    }
    free(processes);
}