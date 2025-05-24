#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <features.h>
#include "uvmTop.h"
#include <pthread.h>
#include <signal.h>

#ifndef SYS_pidfd_getfd
#define SYS_pidfd_getfd 438
#endif

//unsigned long* counterbuffer[45][NVIDIA_MAX_PROCESSOR];
//static int is_event_tracker_setup[45][NVIDIA_MAX_PROCESSOR];
static unsigned int uvm_pids[45]; 
int mode=1;

int get_uvm_fd(int pid){
    //int pid_fd = syscall(SYS_pidfd_open,pid,0);

    //nvidia_uvm fd

    int nvidia_uvm_fd=-1;

    DIR *d;
    struct dirent *dir;
    char psf_path[2048];
    char proc_dir[1024];
    char *psf_realpath;

    sprintf(proc_dir,"/proc/%d/fd",pid);
    d = opendir(proc_dir);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {   //printf("%d\n",dir->d_type);
            if (dir->d_type == 10)
            {
                sprintf(psf_path, "%s/%s",proc_dir, dir->d_name);
                //printf("%s\n",psf_path);
                psf_realpath = realpath(psf_path, NULL);
                //printf("%s\n",psf_realpath);
                if (psf_realpath &&  strcmp(psf_realpath, NVIDIA_UVM_PATH) == 0)
                  {  nvidia_uvm_fd = atoi(dir->d_name);
                    }
                free(psf_realpath);
                if (nvidia_uvm_fd >= 0)
                    break;
            }
        }
        closedir(d);
    }
    return nvidia_uvm_fd ;
}


void* compute_output(void* arg){

    control_fetch_params* process = (control_fetch_params*)arg;
    uuid* uuids=(uuid*)calloc(NVIDIA_MAX_PROCESSOR,sizeof(uuid));
    
    UVM_TOOLS_INIT_EVENT_TRACKER_PARAMS event_tracker;
    UVM_TOOLS_GET_PROCESSOR_UUID_TABLE_PARAMS ioctl_input;
    UVM_TOOLS_GET_CURRENT_COUNTER_VALUES_PARAMS current_value_fetch;
    int pid = process->pid;
    int j=process->pid;
    //printf("Pthread created for %d,%d\n",process->index,pid);
   fflush(stdout);
   //sleep(1);
    int pid_fd = syscall(SYS_pidfd_open,process->pid,0);

    int process_uvm_fd=get_uvm_fd(process->pid);

    if(process_uvm_fd==-1){
        printf("some error happen.....");
	return NULL;
    }

    UVM_TOOLS_ENABLE_COUNTERS_PARAMS counter_enable;

    counter_enable.counterTypeFlags =  UVM_COUNTER_NAME_FLAG_BYTES_XFER_HTD | 
	                               UVM_COUNTER_NAME_FLAG_BYTES_XFER_DTH | 
				       UVM_COUNTER_NAME_FLAG_CPU_PAGE_FAULT_COUNT | 
				       UVM_COUNTER_NAME_FLAG_GPU_PAGE_FAULT_COUNT |
                                       UVM_COUNTER_NAME_FLAG_GPU_EVICTION_COUNT | 
				       UVM_COUNTER_NAME_FLAG_GPU_RESIDENT_COUNT | 
				       UVM_COUNTER_NAME_FLAG_CPU_RESIDENT_COUNT | 
				       UVM_COUNTER_NAME_FLAG_GPU_MEMORY_ALLOCATED |
                                       UVM_COUNTER_NAME_FLAG_OTHER_PROCESS_GPU_MEMORY_EVICTED | 
				       UVM_COUNTER_NAME_THRASHING_PAGES;


    ioctl_input.tablePtr=(unsigned long)uuids;
    volatile int process_fd=syscall(SYS_pidfd_getfd,pid_fd,process_uvm_fd,0);
    //volatile int process_fd = process_uvm_fd;
    ioctl_input.uvmFd=process_fd;
    //printf("%d,%d\n",pid,process_fd);
    int uvm_tools_fd[NVIDIA_MAX_PROCESSOR+1];
    
    //printf("%d",uuids[1].uuid[15]);

    uvm_tools_fd[0]=openat(AT_FDCWD,NVIDIA_UVM_TOOLS_PATH, O_RDWR|O_CLOEXEC);
    ioctl(uvm_tools_fd[0],UVM_IOCTL_TOOLS_GET_GPUs_UUID,(void*)&ioctl_input);
    
    for(;j>0;j=process->pid){
        //sleep(2);
        for(int i=0;i<NVIDIA_MAX_PROCESSOR;i++){
            
            if(uuids[i].uuid[0] && !process->is_event_tracker_setup[i]){
                uvm_tools_fd[i+1]=openat(AT_FDCWD,NVIDIA_UVM_TOOLS_PATH, O_RDWR|O_CLOEXEC);
                
                process->counterbuffer[i]=(unsigned long int*)aligned_alloc(4096,16*sizeof(unsigned long int));
                current_value_fetch.device_id=i;
                current_value_fetch.tablePtr=(unsigned long)process->counterbuffer[i];
                
                /*printf("%d\n",counterbuffer[i][0]);
                printf("%lu\n",2);
                printf("%d\n",2);*/
                event_tracker.allProcessors=0;
                event_tracker.controlBuffer=(unsigned long)process->counterbuffer[i];
                event_tracker.queueBufferSize=0;
                event_tracker.processor=uuids[i];
                event_tracker.uvmFd=process_fd;
                //printf("IOCTL setup for fd %d,%d,%d\n",process->index,process->pid, process_fd);
                fflush(stdout);
                ioctl(uvm_tools_fd[i+1],UVM_IOCTL_TOOLS_INIT_EVENT_TRACKER,(void*)&event_tracker);
                //printf("%u,%ld",event_tracker.rmStatus,sizeof(unsigned long));

                process->is_event_tracker_setup[i]=1;

                ioctl(uvm_tools_fd[i+1],UVM_TOOLS_GET_CURRENT_COUNTER_VALUES ,(void*)&current_value_fetch);
                

                ioctl(uvm_tools_fd[i+1],UVM_IOCTL_TOOLS_ENABLE_COUNTERS,(void*)&counter_enable);
                //process->counterbuffer[i][13]=process->pid;
                //printf("%u,%ld",counter_enable.rmStatus,sizeof(unsigned long));

            }


            /*if(is_event_tracker_setup[process->index][i]){
                if(i==0){  
                printf("| %-*lu | %-*lu | %-*lu | %-*lu | %-*lu |\n",16,i,16,*((unsigned long*)counterbuffer[i]+2),16,0,16,*((unsigned long*)counterbuffer[i]+12),32,*((unsigned long*)counterbuffer[i]+13));
                fflush(stdout);
                }
                else{
                printf("| %-*lu | %-*lu | %-*lu | %-*lu | %-*lu |\n",16,i,16,*((unsigned long*)counterbuffer[i]+9),16,*((unsigned long*)counterbuffer[i]+10),16,*((unsigned long*)counterbuffer[i]+11),32,*((unsigned long*)counterbuffer[i]+13));
                fflush(stdout);
                }
                    //printf("%d,%llu\n",i,*((unsigned long*)counterbuffer[i]+13));
            }*/
        
            //fflush(stdout);
        }

        j=process->pid;

        if(!mode){
            break;
         }
    }
    return NULL;
}

#define LOG_AND_PRINT(...)                              \
    do {                                                \
        printf(__VA_ARGS__);                            \
        if (logfile) {                                  \
            fprintf(logfile, __VA_ARGS__);              \
        }                                               \
    } while (0)

void* print_output(void* arg){
    thread_issued_params* issued_params = (thread_issued_params*)arg;
    long last_written_offset = 0;
    int last_written_pid = -1, last_pid = -1, last_pid_count = 0, last_written_count = 0;
    FILE* logfile = fopen("log.txt", "r+");
    if (!logfile) logfile = fopen("log.txt", "w+");
    if (!logfile) {
        perror("log file");
        return NULL;
    }

    while(1){
        system("clear");
        int i=0;
        int rows=0;

        /* if last pids are different then the process has changed */
        if ((last_written_pid != last_pid) && (last_pid_count != 0) && (last_pid != 0)) {
            fseek(logfile, 0, SEEK_END);        // append
            last_written_offset = ftell(logfile);
        }
        else {
            fseek(logfile, last_written_offset, SEEK_SET);
        }
        last_written_pid = last_pid;
        last_pid = 0;
        last_written_count = last_pid_count;
        last_pid_count = 0;

        while(issued_params[i].is_issued==1){
 
            LOG_AND_PRINT("| Process ID: %-*u |\n",8,issued_params[i].params.pid);
            fflush(stdout);
            LOG_AND_PRINT("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |\n",16,"Processor Id",16,"Number of Faults",
			    16,"Evictions",16,"Resident Pages",32,"Physical Memory allocatd",39,"Memory Evicted of other Processes",
			    16,"Thrashed Pages" );
            fflush(stdout);
            last_pid = issued_params[i].params.pid;
            last_pid_count++;

            for(int j=0;j<NVIDIA_MAX_PROCESSOR;j++){
                if(issued_params[i].params.is_event_tracker_setup[j]){
                    if(j==0){  
                        LOG_AND_PRINT("| %-*u | %-*lu | %-*lu | %-*lu | %-*lu | %-*lu | %-*lu |\n",16,j,16,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+2),16,(unsigned long)0,16,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+12),32,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+13),39,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+14),16,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+15));
                        fflush(stdout);
                    }
                    else{
                        LOG_AND_PRINT("| %-*u | %-*lu | %-*lu | %-*lu | %-*lu | %-*lu | %-*lu |\n",16,j,16,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+9),16,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+10),16,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+11),32,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+13),39,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+14),16,
					*((unsigned long*)issued_params[i].params.counterbuffer[j]+15));
                        fflush(stdout);
                    }
                    rows++;
                    //printf("%d,%llu\n",i,*((unsigned long*)counterbuffer[i]+13));
                 }
            }
            LOG_AND_PRINT("\n");
            LOG_AND_PRINT("\n");
            i++;
        }

        for(int k=0;k<i*(27+112+42+19)+(115+42+19)*rows;k++){
            printf("\b");
            fflush(stdout);
        }
    }
}

void main(int argc, char* argv[]){
    int out_put_file_fd;
    if(argc==1){
        printf("<please specify the mode> -w (watch) or -o <file>(output)\n");
        return ;
    }

    if(!strcmp(argv[1],"-w")){
        mode=1;
    }
    else{
        if(!argv[2]){
            printf("specify name of output file\n");
            return ;
        }
        mode=0;
        out_put_file_fd=open(argv[2],O_RDWR |  O_CREAT | O_TRUNC, 0666);
        dup2(out_put_file_fd,STDOUT_FILENO);
    }

    UVM_TOOLS_GET_UVM_PIDS_PARAMS ioctl_input;
    thread_issued_params* issued_params = calloc(45,sizeof(thread_issued_params));
    if(mode){
        pthread_t thread;
        pthread_create(&thread,NULL,print_output,(void*)issued_params);
    }
    //unsigned int* uvm_pids = (unsigned int*)calloc(45,sizeof(unsigned int));
    ioctl_input.tablePtr=(unsigned long)uvm_pids;
    int uvm_tools_fd=openat(AT_FDCWD,NVIDIA_UVM_TOOLS_PATH, O_RDWR|O_CLOEXEC);

    sleep(2);
    while(1){
        ioctl(uvm_tools_fd,UVM_TOOLS_GET_UVM_PIDS,(void*)&ioctl_input);

        //printf("ioctl_done\n");
        fflush(stdout);

        for(int i=0; uvm_pids[i]!=0;i++){
            //printf("%u\n",uvm_pids[i]);
            fflush(stdout);
            if(issued_params[i].is_issued && issued_params[i].params.pid!=uvm_pids[i]){
                int j;
                for(j=i;issued_params[j].params.pid!=uvm_pids[j] && j<45;j++);
                if(j<45){
    
                    //pthread_kill(issued_params[i].thread,9);
                    issued_params[i]=issued_params[j];
                    issued_params[j].is_issued=0;
                }
                else{
                    issued_params[i].is_issued=0;
                }
               // counterbuffer[i]=(unsigned long)counterbuffer[j];
            }

            //printf("%u\n",uvm_pids[i]);

            if(!issued_params[i].is_issued){

                //printf("%d,%u\n",i,uvm_pids[i]);
                fflush(stdout);
                control_fetch_params* params=(control_fetch_params*)malloc(sizeof(control_fetch_params));
                pthread_t thread;
                params->pid=uvm_pids[i];
                params->index=i;
                params->counterbuffer=(unsigned long**)calloc(NVIDIA_MAX_PROCESSOR,sizeof(unsigned long*));
                params->is_event_tracker_setup=(int*)calloc(NVIDIA_MAX_PROCESSOR,sizeof(int));;
                /*printf("=====================%d,%u==================================\n",i,uvm_pids[i]);
                fflush(stdout);*/
                //sleep(2);
                pthread_create(&thread,NULL,compute_output,(void*)params);
                //sleep(2);
                /*printf("=====================%d,%u==================================\n",i,uvm_pids[i]);
                fflush(stdout);*/
                issued_params[i].is_issued=1;
                issued_params[i].params=*params;

                issued_params[i].thread=thread;
                /*printf("=====================%d,%u==================================\n",i,uvm_pids[i]);
                fflush(stdout);*/
            }
        }

        if(uvm_pids[0]==0 && issued_params[0].is_issued==1){
            issued_params[0].is_issued=0;
            // pthread_kill(issued_params[0].thread,9);
        }

        if(!mode){
            break;
        }
	else {
	    struct timeval timeout;
            fd_set readfds;
	    char ch;

	    fflush(stdout);

            FD_ZERO(&readfds);
            FD_SET(0, &readfds); // file descriptor 0 = stdin

            timeout.tv_sec = 2;  // 1 second
            timeout.tv_usec = 0;
    
            int ret = select(1, &readfds, NULL, NULL, &timeout);

            if (ret > 0) {
                // input is ready
                read(STDIN_FILENO, &ch, 1);
                if (ch == 'q' || ch == 'Q') {
                    printf("\nExiting...\n");
                    break;
                }
            }
	}

    }
    int k=0;
    printf("| Process ID | CPU/GPU | %s | %s | %s | %s | %s | %s | %s |\n","Processor Id","Number of Faults",
			    "Evictions","Resident Pages","Physical Memory allocatd","Memory Evicted of other Processes",
			    "Thrashed Pages" );
    while(issued_params[k].is_issued==1){
        pthread_join(issued_params[k].thread,NULL);
        for(int j=0;j<NVIDIA_MAX_PROCESSOR;j++){
            if(issued_params[k].params.is_event_tracker_setup[j]){
                if(j==0){  
                    printf("%d,%d,%lu,%lu,%lu,%lu,%lu,%lu\n",issued_params[k].params.pid,j,
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+2),(unsigned long)0,
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+12),
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+13),
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+14),
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+15));
                }
                else{
                    printf("%d,%d,%lu,%lu,%lu,%lu,%lu,%lu\n",issued_params[k].params.pid,j,
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+9),
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+10),
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+11),
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+13),
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+14),
                        *((unsigned long*)issued_params[k].params.counterbuffer[j]+15));
                }
            }
        }
        k++;
    }
    return;
}
