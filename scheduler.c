#include "headers.h"
#include "ProcessQueue.h"
#include "PCB.h"
#include <string.h>
#include <sys/wait.h>

struct msgbuff
{
    long mtype;
    Process mtext;
};

// Global Variables
Process *Running_Process = NULL; // Pointer to the currently running process
int scheduling_algorithm;  
int Nprocesses;               // Total number of processes passed from process generator
int handled_processes_count;  // Total number of actually scheduled processes 
int roundRobinSlice; 
int active_time;                // Tracks CPU active time for performance calculation       
FILE *schedulerLog, *perfLog; 
ProcessQueue ready_list;      // Ready queue to store processes

// Function Prototypes
void init_Scheduler(int argc, char *argv[]);
void handle_process_reception(int msg_queue_id, ProcessQueue *ready_list);
void calculate_performance(float *allWTA, int *allWT, int handled_processes_count, int totalRunTime);
void log_event(const char *event, Process *process);
void run(Process *process);
void fork_process(Process *process);
void handle_process_stop(int qid_process, float *allWTA, int *allWT);
void handle_process_completion(Process * process);
void handle_HPF();
void handle_SJF();
void handle_MLFQ();
void handle_RR();

int main(int argc, char *argv[])
{
    initClk();

    // Initialize scheduler
    init_Scheduler(argc, argv);
    init_PCB(); // Initialize PCB table

    // Message queues
    key_t key_id1 = ftok("keyfile", 70);
    int qid_generator = msgget(key_id1, 0666 | IPC_CREAT); // From process generator

    init_ProcessQueue(&ready_list); // Initialize ready queue

    int start_time = getClk(); 
    float *allWTA = calloc(Nprocesses, sizeof(float)); // Weighted Turnaround Times (dynamic)
    int *allWT = calloc(Nprocesses, sizeof(int));      // Waiting Times (dynamic)

    int prevClk = getClk();
    int currentClk;
    // Main scheduler loop
    while (1) 
    {
        currentClk = getClk();
        if(currentClk != prevClk)
        {
            // Check if all processes are completed
            if (Nprocesses == handled_processes_count) 
            {
                int end_time = getClk();
                int totalRunTime = end_time - start_time;
                calculate_performance(allWTA, allWT, handled_processes_count, totalRunTime); // Compute metrics
                break; // Exit scheduler loop
            }

            // Handle new arrivals from the generator
            handle_process_reception(qid_generator, &ready_list);

            // Select and execute the appropriate scheduling algorithm
            switch (scheduling_algorithm) 
            {
                case 1:
                    handle_SJF();
                    break;
                case 2:
                    handle_HPF();
                    break;
                case 3:
                    handle_RR();
                    break;
                case 4: 
                    handle_MLFQ();
                    break;
                default:
                    fprintf(stderr, "Invalid scheduling algorithm\n");
                    exit(EXIT_FAILURE);
            }
            currentClk = getClk();
            prevClk = currentClk;
        }
    }

    // Clean up and exit
    free(allWTA);
    free(allWT);
    fclose(schedulerLog);
    fclose(perfLog);
    msgctl(qid_generator, IPC_RMID, NULL);
    free_PCB(); // Free dynamically allocated PCB table
    destroyClk(true);
    return 0;
}

// Initialize scheduler configurations and open log files
void init_Scheduler(int argc, char *argv[]) 
{
    scheduling_algorithm = atoi(argv[1]); 
    Nprocesses = atoi(argv[2]);
    roundRobinSlice = (argc > 3) ? atoi(argv[3]) : 0; // Time slice for RR if provided
    handled_processes_count = 0;
    active_time = 0;

    // Open log files
    schedulerLog = fopen("Scheduler.log", "w");
    perfLog = fopen("Scheduler.perf", "w");
    if (!schedulerLog || !perfLog) 
    {
        fprintf(stderr, "Failed to open log files\n");
        exit(EXIT_FAILURE);
    }
    fprintf(schedulerLog, "#AT time x process y state arr w total z remain y wait k\n");
}

// Receive new processes from the process generator
void handle_process_reception(int msg_queue_id, ProcessQueue *ready_list) 
{
    struct msgbuff message;
    while (msgrcv(msg_queue_id, &message, sizeof(Process), 0, IPC_NOWAIT) != -1) 
    {
        active_time += message.mtext.runtime;
        enqueue_ProcessQueue(ready_list, message.mtext); // Add to ready queue
        fork_process(&message.mtext); //fork the process
    }
}

void fork_process(Process *process)
{
    int pid = fork();
    if(pid!=0) // Parent (Scheduler)
    { process->pid = pid; } // Store PID in PCB
}

// Fork and run the process for a specified runtime, handling "resumed" events
void run(Process *process) //called inside scheduling algorithms
{
    if(process->pid == getpid())
    {
        add_to_PCB(process); // Add the process to the PCB table
        if (process->state == 1) // Previously stopped (waiting state)
        { log_event("resumed", process); } // Log resumed event
        else // Child process
        { log_event("started", process); } // Log started event

        // execv("./process.out", NULL);
        Running_Process = process; //can be removed later
    }
}

// Handle process preemption
void handle_process_stop(Process * process)
{
    // Preempted process, re-enqueue
    process->state = 1; // Waiting state
    enqueue_ProcessQueue(&ready_list, process); //return to ready list -> debatable
    log_event("stopped", &process); // Log stopped event
    Running_Process = NULL; //can be removed later
}

// Handle process completion 
void handle_process_completion(Process * process)
{
    // Process completed
    process->remainingtime = 0;
    process->finishtime = getClk();
    process->TA = process->finishtime - process->arrivaltime;
    process->WTA = (float)process->TA / process->runtime;
    process->waitingtime = process->TA - process->runtime;

    // Populate allWTA and allWT arrays using process ID as the index
    allWTA[process->id] = process->WTA;
    allWT[process->id] = process->waitingtime;

    log_event("finished", process); // Log finished event
    remove_from_PCB(process->pid); // Remove from PCB table
    handled_processes_count++;
    if(process->pid == getpid)
    { exit(0); } //process terminating itself -> should be in process.c ?

    Running_Process = NULL; //can be removed later
}

// Log an event with relevant details
void log_event(const char *event, Process *process)
{
    if (strcmp(event, "started") == 0 || strcmp(event, "resumed") == 0 || strcmp(event, "stopped") == 0) 
    {
        fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n",
                getClk(), process->id, event, process->arrivaltime, process->runtime, process->remainingtime, process->waitingtime);
    } 
    else if (strcmp(event, "finished") == 0) 
    {
        fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
                getClk(), process->id, event, process->arrivaltime, process->runtime, process->remainingtime, process->waitingtime, process->TA, process->WTA);
    }
}

// Calculate performance metrics
void calculate_performance(float *allWTA, int *allWT, int handled_processes_count, int totalRunTime)
{
    float avgWTA = 0;
    float avgWT = 0;
    float cpuUtil = (active_time * 100.0) / totalRunTime;

    for (int i = 0; i < handled_processes_count; i++)
    {
        avgWTA += allWTA[i];
        avgWT += allWT[i];
    }
    avgWTA /= handled_processes_count;
    avgWT /= handled_processes_count;

    fprintf(perfLog, "CPU utilization = %.2f%%\n", cpuUtil);
    fprintf(perfLog, "Avg WTA = %.2f\n", avgWTA);
    fprintf(perfLog, "Avg Waiting Time = %.2f\n", avgWT);
}
