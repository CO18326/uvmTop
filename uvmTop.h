/**************************************************************************************************************

current implementaion is not supporting mig

************************************************************************************************************************** */

#define NVIDIA_UVM_TOOLS_PATH "/dev/nvidia-uvm-tools"
#define NVIDIA_UVM_PATH "/dev/nvidia-uvm"


//max processor allowed...... value copied from the driver
#define NVIDIA_MAX_PROCESSOR 32*8+1

#define UVM_IOCTL_TOOLS_GET_GPUs_UUID 80
#define UVM_IOCTL_TOOLS_INIT_EVENT_TRACKER 56
#define UVM_IOCTL_TOOLS_ENABLE_COUNTERS  60 
#define UVM_TOOLS_GET_CURRENT_COUNTER_VALUES  81
#define UVM_TOOLS_GET_UVM_PIDS  82



#define UVM_COUNTER_NAME_FLAG_BYTES_XFER_HTD 0x1
#define UVM_COUNTER_NAME_FLAG_BYTES_XFER_DTH 0x2
#define UVM_COUNTER_NAME_FLAG_CPU_PAGE_FAULT_COUNT 0x4
#define UVM_COUNTER_NAME_FLAG_WDDM_BYTES_XFER_BTH 0x8
#define UVM_COUNTER_NAME_FLAG_WDDM_BYTES_XFER_HTB 0x10
#define UVM_COUNTER_NAME_FLAG_BYTES_XFER_DTB 0x20
#define UVM_COUNTER_NAME_FLAG_BYTES_XFER_BTD 0x40
#define UVM_COUNTER_NAME_FLAG_PREFETCH_BYTES_XFER_HTD 0x80
#define UVM_COUNTER_NAME_FLAG_PREFETCH_BYTES_XFER_DTH 0x100
#define UVM_COUNTER_NAME_FLAG_GPU_PAGE_FAULT_COUNT 0x200
#define UVM_COUNTER_NAME_FLAG_GPU_EVICTION_COUNT 0x400
#define UVM_COUNTER_NAME_FLAG_GPU_RESIDENT_COUNT 0x800
#define UVM_COUNTER_NAME_FLAG_CPU_RESIDENT_COUNT 0x1000
#define UVM_COUNTER_NAME_FLAG_GPU_MEMORY_ALLOCATED 0x2000
#define UVM_COUNTER_NAME_FLAG_OTHER_PROCESS_GPU_MEMORY_EVICTED 0x4000
#define UVM_COUNTER_NAME_THRASHING_PAGES      0x8000






typedef struct
{
    unsigned long     tablePtr                 __attribute__ ((aligned (8))); // IN
    unsigned int      uvmFd                     ;
    unsigned int  rmStatus;                                                   // OUT
} UVM_TOOLS_GET_PROCESSOR_UUID_TABLE_PARAMS;



typedef struct
{
    unsigned char uuid[16];

} uuid;


typedef struct
{
    unsigned long           queueBuffer         __attribute__ ((aligned (8))); // IN
    unsigned long           queueBufferSize     __attribute__ ((aligned (8))); // IN
    unsigned long           controlBuffer        __attribute__ ((aligned (8))); // IN
    uuid                     processor;                            // IN
    unsigned int             allProcessors;                        // IN
    unsigned int             uvmFd;                                // IN
    unsigned int             rmStatus;                             // OUT
} UVM_TOOLS_INIT_EVENT_TRACKER_PARAMS;


typedef struct
{
    unsigned long     counterTypeFlags          __attribute__ ((aligned (8))); // IN
    unsigned int      rmStatus;                                    // OUT
} UVM_TOOLS_ENABLE_COUNTERS_PARAMS;



typedef struct
{
    unsigned long     tablePtr                 __attribute__ ((aligned (8)));                 // IN
    unsigned int      device_id                              ;
    unsigned int      rmStatus;                                   // OUT
} UVM_TOOLS_GET_CURRENT_COUNTER_VALUES_PARAMS;



typedef struct
{
    unsigned long     tablePtr                 __attribute__ ((aligned (8)));     // IN 
    unsigned int     rmStatus;                                   // OUT
} UVM_TOOLS_GET_UVM_PIDS_PARAMS;


typedef struct{

unsigned int pid;
int index;
unsigned long ** counterbuffer;
int* is_event_tracker_setup;

}control_fetch_params;


typedef struct{
    control_fetch_params params;
    int is_issued;
    pthread_t thread;
}thread_issued_params;



int get_uvm_fd(int pid);
void setup_eventtracker(uuid gpu_uuid,unsigned long* controlBuffer);