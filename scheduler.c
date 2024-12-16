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
pid_t scheduler_pid;

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
void handle_RR(ProcessQueue *ready_queue,int quatnum,  float *allWTA, int *allWT);

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
    scheduler_pid = getpid();
    printf("ANA SCHEDULER W DA EL PID BTA3YYYYY %d\n", scheduler_pid);
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
                    handle_RR(&ready_list,roundRobinSlice,allWTA,allWT);
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
    kill(getppid(),SIGINT); //check this for el tarteeb
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
        // fork_process(&message.mtext); //fork the process
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
                    enqueue_linkedlist(&levels[priority_level], message.mtext);
                }
                break;
            default:
                fprintf(stderr, "Invalid scheduling algorithm\n");
                exit(EXIT_FAILURE);
        }
    }
}

void fork_process(Process *process)
{
    if(getpid()==scheduler_pid) //only scheduler can fork new processes
    {
        printf("ana fl fork\n");
        pid_t pid = fork();
        printf("da el pid el ana 3mltlo fork w da ana %d %d \n", pid, getpid());
        if(pid==0)
        {
            process->pid = getpid();
            printf("ana ray7 a.run process.c\n");
            char remtime[20];
            sprintf(remtime, "%d", process->remainingtime);
            char *args[] = {"./process.out", remtime, NULL};
            execv("./process.out", args);
        }
    }
}

// Fork and run the process for a specified runtime, handling "resumed" events
void run(Process *process) //called inside scheduling algorithms
{
    if (process->state == 1) // Previously stopped (waiting state)
    { 
        log_event("resumed", process); //why is this never logged ?
    } // Log resumed event
    else if(process->remainingtime == process->runtime)// starting for the first time
    { 
        fork_process(process);
        log_event("started", process); 
        add_to_PCB(process); // Add the process to the PCB table
    } // Log started event
    process->state = 0;
    kill(process->pid,SIGINT);
    printf("ana fl run w el pid bta3y %d %d\n", process->pid, getpid());
    process->remainingtime--;
    Running_Process = process; //can be removed later
}

// Handle process preemption
void handle_process_stop(Process * process)
{
    // Preempted process, re-enqueue
    process->state = 1; // Waiting state
    log_event("stopped", process); // Log stopped event
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
            break;
        default:
            fprintf(stderr, "Invalid scheduling algorithm\n");
            exit(EXIT_FAILURE);
    }
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
    printf("process:%d completed\n",process->pid);
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
////////////////////////////////////////////////////////////////////////////////RR//////////////////////////////////////////////////////
void handle_RR(ProcessQueue *ready_queue, int quatnum,  float *allWTA, int *allWT)
{
    // Static variable to count the quantum time
    static int quantum_counter = 0;

    // Check if the ready queue is empty
    if (isEmpty_ProcessQueue(ready_queue)) {
        return;  // No process to run
    }

    // Run the fisrst process from the ready queue
    Process *current_process = peek_ProcessQueue(ready_queue);


     // Check if the process has finished its remaining time
    if (current_process->remainingtime <= 0) {
         // Process has completed
         Process *completed_process = dequeue_ProcessQueue(ready_queue);
         handle_process_completion(completed_process, allWTA, allWT);
         quantum_counter = 0;  // Reset the quantum counter for the next round of scheduling
    } 
    // Check if the process's quantum has been fully used (i.e., 1 time slice = quantum step)
    else if(quantum_counter == quatnum) {
        // Process is not finished, preempt it and place it back in the ready queue
         Process *preempted_process = dequeue_ProcessQueue(ready_queue);
         handle_process_stop(preempted_process);
        // enqueue_ProcessQueue(ready_queue, *preempted_process);  // Re-enqueue the process for later execution
         quantum_counter = 0;  // Reset the quantum counter for the next round of scheduling
     }


    if (isEmpty_ProcessQueue(ready_queue)) {
        return;  // No process to run
    }
 
    // Run the fisrst process from the ready queue
    current_process = peek_ProcessQueue(ready_queue);


    // Calculate the quantum for this process (minimum of remaining time and time slice)
    int time_to_run = (current_process->remainingtime < quatnum) ? current_process->remainingtime : quatnum;
    
    // Run the process for one time slice (1 time step)
    run(current_process);  // Call the run function to simulate process execution for 1 time unit
     // Debugging: print the PID when the process is added
    printf("Running process with PID: %d to PCB\n", current_process->pid);

    
    // Increment the quantum counter
    quantum_counter++;
}

/////////////////////////////////////////////// MLFQ ///////////////////////////////////////////////////////////////////////////
// Initialize all levels to empty linked lists
void init_MLFQ() {
    for (int i = 0; i < NUM_LEVELS; i++) {
        levels[i] = NULL;  // Initialize each level to NULL (empty linked list)
    }
}

void handle_MLFQ(float *allWTA, int *allWT, int time_quantum) {
    static int time_remaining = 0;
    static int processesRunInLevel10 = 0;  
    static int currentLevel = 0;

    printf("Clock: %d\n", getClk());

    // Select the highest-priority process
    if (Running_Process == NULL) {
        for (int i = 0; i < NUM_LEVELS; i++) {
            if (!isLevelEmpty(levels[i])) {
                Running_Process = malloc(sizeof(Process));
                if (Running_Process == NULL) {
                    fprintf(stderr, "Memory allocation failed for Running_Process\n");
                    exit(EXIT_FAILURE);
                }
                *Running_Process = dequeue_linkedlist(&levels[i]);
                currentLevel = i;  // Update the current level
                time_remaining = (Running_Process->remainingtime < time_quantum)
                                 ? Running_Process->remainingtime
                                 : time_quantum;
                printf("Selected process with ID = %d from level %d\n", Running_Process->id, currentLevel);
                break;
            }
        }
        if(currentLevel==10 && Running_Process!=NULL){
            processesRunInLevel10++;
        }
    }

    // If no process is selected, all queues are empty
    if (Running_Process == NULL) {
        printf("No processes available to run.\n");
        return; // Nothing to execute
    }

    print_process(*Running_Process); //3yza ashof hagat extra

    // Run the selected process
    if (Running_Process->remainingtime > 0) {
        printf("Running process with ID = %d, time_remaining = %d, level = %d\n", Running_Process->id, time_remaining, currentLevel);
        run(Running_Process);
        time_remaining--;
    }
    // Handle process completion
    if (Running_Process->remainingtime <= 0) {
        printf("Process with ID = %d completed, remaining time = %d\n", Running_Process->id, Running_Process->remainingtime);
        handle_process_completion(Running_Process, allWTA, allWT);
        free(Running_Process);
        Running_Process = NULL;
        time_remaining = 0;
    }
    // Handle process preemption (time slice expired)
    else if (time_remaining == 0) {
        //printf("Process with ID = %d preempted, demoting to next level.\n", Running_Process->id);
        if (currentLevel + 1 < NUM_LEVELS) {
            enqueue_linkedlist(&levels[currentLevel + 1], *Running_Process);
            //printf("Demoted process with ID = %d to level %d\n", Running_Process->id, currentLevel + 1);
        } else {
            enqueue_linkedlist(&levels[10], *Running_Process);
            //printf("Process with ID = %d moved to level 10\n", Running_Process->id);
        }
        handle_process_stop(Running_Process);
        if (currentLevel==10)
        {
            printf("Process runned = %d , level size = %d \n", processesRunInLevel10 , getLevelSize(levels[10]));
        }
        
        if (currentLevel == 10 && processesRunInLevel10 == getLevelSize(levels[10])) {
            redistributeProcessesByPriority();
            processesRunInLevel10 = 0; 
        }
        
        free(Running_Process);
        Running_Process = NULL;
        time_remaining = 0;  // Reset time slice
    }

}

void redistributeProcessesByPriority() {
    printf("DISTRUBUTEEE \n");
    Node *dummy_level10 = NULL;
    int level10_size = getLevelSize(levels[10]);

    while (!isLevelEmpty(levels[10])) {
        Process process = dequeue_linkedlist(&levels[10]);
        int new_priority_level = process.priority;

        if (new_priority_level != 10) {
            enqueue_linkedlist(&levels[new_priority_level], process); // Enqueue directly
        } else {
            enqueue_linkedlist(&dummy_level10, process); // Add to dummy list
        }
    }

    levels[10] = dummy_level10; // Assign updated level 10
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
