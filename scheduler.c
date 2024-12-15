#include "headers.h"
#include "ProcessQueue.h"
#include "PCB.h"
#include <string.h>
#include <sys/wait.h>
#include "LinkedList.h"

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
void handle_process_stop(Process * process);
void handle_process_completion(Process * process, float *allWTA, int *allWT);
void handle_HPF();
void handle_SJF();
void handle_RR();

/////////////////////////////////////////////////////////MLFQ////////////////////////////////////////////////////////////
// Declare and initialize queues for each level
Node *levels[NUM_LEVELS];

// Function Prototypes
void init_MLFQ(); 
void handle_MLFQ(float *allWTA, int *allWT ,int time_quantum);
void redistributeProcessesByPriority(); 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



int main(int argc, char *argv[])
{
    initClk();

    // Initialize scheduler
    init_Scheduler(argc, argv);
    init_PCB(); // Initialize PCB table

    // Message queues
    key_t key_id1 = ftok("keyfile", 70);
    int qid_generator = msgget(key_id1, 0666 | IPC_CREAT); // From process generator

    init_ProcessQueue(&ready_list, Nprocesses); // Initialize ready queue (MIRA/REHAB: Hasa dy el mafrod tb2a gowa el init scheduler el switch bsr7a By mimo)

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
                    handle_MLFQ(allWTA,allWT,roundRobinSlice);
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
    kill(SIGINT, getppid());
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

    // Intializing the appropraite DS for the scheduling
    switch (scheduling_algorithm) 
    {
        case 1:
            //TODO: init SJF (Dandon);
            break;
        case 2:
            //TODO: init HPF (Dandon);
            break;
        case 3:
            //TODO: init RR (Rehab); //Ready queue 
            break;
        case 4: 
            init_MLFQ();
            break;
        default:
            fprintf(stderr, "Invalid scheduling algorithm\n");
            exit(EXIT_FAILURE);
    }
}

// Receive new processes from the process generator
void handle_process_reception(int msg_queue_id, ProcessQueue *ready_list) 
{
    //add switch case for ready_list per each algorithm
    struct msgbuff message;
    while (msgrcv(msg_queue_id, &message, sizeof(Process), 0, IPC_NOWAIT) != -1) 
    {
        active_time += message.mtext.runtime;
        switch (scheduling_algorithm) 
        {
            case 1:
                //TODO: insert in SJF (Dandon);
                break;
            case 2:
                //TODO: insert in HPF (Dandon);
                break;
            case 3:
                enqueue_ProcessQueue(ready_list, message.mtext); // Add to ready queue
                break;
            case 4: 
                int priority_level = message.mtext.priority; 
                if (priority_level >= 0 && priority_level < NUM_LEVELS) {
                    printf("Should insert in level %d \n", priority_level);
                    enqueue_linkedlist(&levels[priority_level], message.mtext);
                }
                break;
            default:
                fprintf(stderr, "Invalid scheduling algorithm\n");
                exit(EXIT_FAILURE);
        }
        fork_process(&message.mtext); //fork the process
    }
}


void fork_process(Process *process)
{
    int pid = fork();
    if(pid==0)
    {
        char remtime[20];
        sprintf(remtime, "%d", process->remainingtime);
        char *args[] = {"./process.out", remtime, NULL};
        execv("./process.out", args);
    }
    if(pid!=0) // Parent (Scheduler)
    { 
        process->pid = pid; 
    } // Store PID in PCB
    
}

// Fork and run the process for a specified runtime, handling "resumed" events
void run(Process *process) // Called inside scheduling algorithms
{
    printf("Inside run \n");
    add_to_PCB(process); // Add the process to the PCB table

    // Log event based on process state
    if (process->state == 1) // Previously stopped (waiting state)
    {
        log_event("resumed", process); // Log resumed event
    }
    else // New process
    {
        log_event("started", process); // Log started event
    }

    // Send SIGUSR1 to the process
    kill(process->pid, SIGUSR1);

    // Update remaining time
    process->remainingtime--;
    Running_Process = process; //can be removed later

    // Print process information
    printf("Process running with id %d, remaining_time %d\n", process->id, process->remainingtime);
}

// Handle process preemption
void handle_process_stop(Process * process)
{
    // Preempted process, re-enqueue
    process->state = 1; // Waiting state
    switch (scheduling_algorithm) 
        {
            case 1:
                //TODO: insert in SJF (Dandon);
                break;
            case 2:
                //TODO: insert in HPF (Dandon);
                break;
            case 3:
                enqueue_ProcessQueue(&ready_list, *process); //return to ready list -> debatable
                break;
            case 4: 
                int priority_level = process->priority; 
                if (priority_level >= 0 && priority_level < NUM_LEVELS) {
                    enqueue_linkedlist(&levels[priority_level], *process);
                }
                break;
            default:
                fprintf(stderr, "Invalid scheduling algorithm\n");
                exit(EXIT_FAILURE);
        }
    log_event("stopped", process); // Log stopped event
    Running_Process = NULL; //can be removed later
}

// Handle process completion 
void handle_process_completion(Process * process, float *allWTA, int *allWT)
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


void handle_HPF()
{
    return;
}
void handle_SJF()
{
    return;
}
/////////////////////////////////////////////// MLFQ ///////////////////////////////////////////////////////////////////////////
// Initialize all levels to empty linked lists
void init_MLFQ() {
    for (int i = 0; i < NUM_LEVELS; i++) {
        levels[i] = NULL;  // Initialize each level to NULL (empty linked list)
    }
}

//pre-emptive ana msh 3yza keda
void handle_MLFQ(float *allWTA, int *allWT, int time_quantum) {
    static int current_level = 0;  // Tracks the current level of the queue
    static int processesRunInLevel10 = 0;  // Counter for processes run in level 10
    static int remaining_run = 0;  // Tracks the remaining time of the current running process
    static Process *running_process = NULL;  // Pointer to the currently running process

    printf("Current clock: %d\n", getClk());

    // If there's no running process or remaining time is exhausted, get a new process
    if (running_process == NULL || remaining_run == 0) {
        // Find the next non-empty level to process
        while (current_level < NUM_LEVELS && isLevelEmpty(levels[current_level])) {
            current_level++;
        }

        // Cap to the highest valid level if necessary
        if (current_level >= NUM_LEVELS) {
            current_level = NUM_LEVELS - 1;  // Cap to the highest valid level
        }

        // Special handling for level 10
        if (allLevelsEmptyExceptLevel10(levels) && current_level == 10) {
            if (processesRunInLevel10 == getLevelSize(levels[10])) { 
                redistributeProcessesByPriority();
                processesRunInLevel10 = 0; 
                current_level = 0; 
            }
        }

        // If no processes remain, return
        if (allLevelsEmpty(levels)) {
            processesRunInLevel10 = 0; 
            return;
        }

        // Get the next process to run from the current level
        running_process = dequeue_linkedlist(&levels[current_level]);
        remaining_run = (running_process->remainingtime < time_quantum) ? running_process->remainingtime : time_quantum;
    }

    if (running_process) {
        // Run the process for the remaining time
        run(running_process);
        remaining_run--;

        // If the process has completed
        if (running_process->remainingtime == 0) {
            handle_process_completion(running_process, allWTA, allWT);
            running_process = NULL;  // Reset running process after completion
        } else if (remaining_run == 0) {
            // Process didn't finish in the current quantum, preempt it and consider demotion or promotion
            handle_process_stop(running_process);

            // Demote to the next level if not at the highest priority
            if (current_level < NUM_LEVELS - 1) {
                enqueue_linkedlist(&levels[current_level + 1], *running_process);  // Demote to next level
            } else {
                // Stay at the current level if it's the lowest level (level 10)
                enqueue_linkedlist(&levels[current_level], *running_process);  
                if (current_level == 10) {
                    processesRunInLevel10++;
                }
            }

            // Reset running process for the next round
            running_process = NULL;
        }
    }

    // Do not reset `current_level` to 0 here if you want the scheduler to continue on the next cycle
}

void redistributeProcessesByPriority() {
    Node *dummy_level10 = NULL;  // This will hold processes that should stay at level 10
    Node *temp = NULL;  // Temporary node for traversal

    // Process the queue for level 10
    int level10_size = getLevelSize(levels[10]);
    while (!isLevelEmpty(levels[10])) {
        // Dequeue the process from level 10
        Process *process = dequeue_linkedlist(&levels[10]);

        if (process) {
            int new_priority_level = process->priority;  // Get the new priority level from the process

            // If the process's priority is not 10, enqueue it to the appropriate level
            if (new_priority_level != 10) {
                enqueue_linkedlist(&levels[new_priority_level], *process);
            } else {
                // Otherwise, add the process to the dummy list (for level 10)
                enqueue_linkedlist(&dummy_level10, *process);
            }
        }
    }

    // After processing, set level 10 to the dummy list
    levels[10] = dummy_level10;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handle_RR()
{
    return;
}
