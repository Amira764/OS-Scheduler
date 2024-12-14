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
int idle_time;                // Tracks CPU idle time for performance calculation       
FILE *schedulerLog, *perfLog; 
ProcessQueue ready_list;      // Ready queue to store processes

// Function Prototypes
void init_Scheduler(int argc, char *argv[]);
void handle_process_reception(int msg_queue_id, ProcessQueue *ready_list);
void calculate_performance(float *allWTA, int *allWT, int handled_processes_count, int totalRunTime);
void log_event(const char *event, Process *process);
void fork_and_run(Process *process, int runtime);
void handle_process_completion(int qid_process, float *allWTA, int *allWT);
void handle_preemption(Process *current_process, int qid_process);
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

    key_t key_id2 = ftok("keyfile", 60);
    int qid_process = msgget(key_id2, 0666 | IPC_CREAT); // From process.c

    init_ProcessQueue(&ready_list); // Initialize ready queue

    int start_time = getClk(); 
    float *allWTA = calloc(Nprocesses, sizeof(float)); // Weighted Turnaround Times (dynamic)
    int *allWT = calloc(Nprocesses, sizeof(int));      // Waiting Times (dynamic)

    // Main scheduler loop
    while (1) 
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

        // Handle process completion or preemption
        handle_process_completion(qid_process, allWTA, allWT);
    }

    // Clean up and exit
    free(allWTA);
    free(allWT);
    fclose(schedulerLog);
    fclose(perfLog);
    msgctl(qid_generator, IPC_RMID, NULL);
    msgctl(qid_process, IPC_RMID, NULL);
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
    idle_time = 0;

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
        enqueue_ProcessQueue(ready_list, message.mtext); // Add to ready queue
    }
}

// Fork and run the process for a specified runtime, handling "resumed" events
void fork_and_run(Process *process, int runtime)
{
    if (process->state == 1) // Previously stopped (waiting state)
    {
        log_event("resumed", process); // Log resumed event
    }

    int pid = fork();
    if (pid == 0) // Child process
    {
        char runtime_arg[10];
        sprintf(runtime_arg, "%d", runtime);
        char *args[] = {"./process.out", runtime_arg, NULL};
        execv("./process.out", args);
    }
    else // Parent (Scheduler)
    {
        process->pid = pid; // Store PID in PCB
        process->state = 0; // Running state
        log_event("started", process); // Log started event
        add_to_PCB(process); // Add the process to the PCB table
        Running_Process = process;
    }
}

// Handle process completion or preemption
void handle_process_completion(int qid_process, float *allWTA, int *allWT)
{
    struct msgbuff message;
    while (msgrcv(qid_process, &message, sizeof(message), 0, IPC_NOWAIT) != -1)
    {
        Process *current_process = Running_Process;
        if (message.mtext.remainingtime > 0) 
        {
            // Preempted process, re-enqueue
            current_process->remainingtime = message.mtext.remainingtime;
            current_process->state = 1; // Waiting state
            enqueue_ProcessQueue(&ready_list, *current_process);
            log_event("stopped", current_process); // Log stopped event
        }
        else 
        {
            // Process completed
            current_process->remainingtime = 0;
            current_process->finishtime = getClk();
            current_process->TA = current_process->finishtime - current_process->arrivaltime;
            current_process->WTA = (float)current_process->TA / current_process->runtime;
            current_process->waitingtime = current_process->TA - current_process->runtime;

            // Populate allWTA and allWT arrays using process ID as the index
            allWTA[current_process->id] = current_process->WTA;
            allWT[current_process->id] = current_process->waitingtime;

            log_event("finished", current_process); // Log finished event
            remove_from_PCB(current_process->pid); // Remove from PCB table
            handled_processes_count++;
        }
        Running_Process = NULL;
    }
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
    float cpuUtil = ((totalRunTime - idle_time) * 100.0) / totalRunTime;

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
