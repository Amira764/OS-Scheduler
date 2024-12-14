#include "headers.h"
#include "ProcessQueue.h"
#include <string.h>

struct msgbuff
{
    long mtype;
    Process mtext;
};

// Global Variables
Process *Running_Process;     // Currently running process -> m3rfsh 3yzenha f haga wla la bs sybahalko just in case 7d e7tagha
int scheduling_algorithm;  
int Nprocesses;               //Total number of processes passed from process generator
int handled_processes_count;  //Total number of actually scheduled processes 
int roundRobinSlice; 
int idle_time;                //please matnsosh t.update.o da kman       
FILE *schedulerLog, *perfLog; 

// Function Prototypes
void init_Scheduler(int argc, char *argv[]);
void handle_process_reception(int msg_queue_id, ProcessQueue *ready_list);
void calculate_performance(float *allWTA, int handled_processes_count, int totalRunTime);
void log_event(const char *event, Process *process); //call it kol m t3mlo event mn el 4 log it 
void handle_HPF();
void handle_SJF();
void handle_MLFQ();
void handle_RR();

int main(int argc, char *argv[])
{
    initClk();

    //TODO: implement the scheduler.
    init_Scheduler(argc, argv);
    key_t key_id = ftok("keyfile", 70);
    int qid = msgget(key_id, 0666 | IPC_CREAT); // message queue to receive processes from process_generator file
    ProcessQueue ready_list;
    init_ProcessQueue(&ready_list); // Temporary queue for new processes

    int start_time = getClk(); 
    float allWTA[1000] = {0}; // Array to store Weighted Turnaround Times

    // Main scheduler loop
    while (1) 
    {
        handle_process_reception(qid, &ready_list); //lw 3yzeen ta5doha gowa el functions bt3tko it's up to u
        // el fn msh bt.fork el process hya bs bt7ottaha fl ready list ana sybalko ento el forking
        // el PCB howa struct Process, matnsoosh t.update it

        // Select the appropriate scheduling algorithm
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

        // Check if all processes are completed
        if (Nprocesses == handled_processes_count) 
        {
            int end_time = getClk();
            int totalRunTime = end_time - start_time;
            calculate_performance(allWTA, Nprocesses, totalRunTime); // Compute metrics
            break; // Exit scheduler loop
        }
    }

    // Clean up and exit
    fclose(schedulerLog);
    fclose(perfLog);
    msgctl(qid, IPC_RMID, NULL); // Remove message queue

    //TODO: upon termination release the clock resources.
    // what are clk resources ?
    destroyClk(true);
    return 0;
}

// Initialize scheduler configurations and open log files
void init_Scheduler(int argc, char *argv[]) 
{
    scheduling_algorithm = atoi(argv[1]); 
    Nprocesses = atoi(argv[2]);
    roundRobinSlice = (argc > 3) ? atoi(argv[3]) : 0; // Get time slice for RR if provided
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
    fprintf(schedulerLog, "#AT time x process y state arr w total z remain y wait k"); //first line in log file
}

// Handle process reception from the message queue
void handle_process_reception(int msg_queue_id, ProcessQueue *ready_list) 
{
    struct msgbuff message;
    while (msgrcv(msg_queue_id, &message, sizeof(Process), 0, IPC_NOWAIT) != -1) // Add received process to ready list
    { enqueue_ProcessQueue(ready_list, message.mtext); }
}

// Calculate performance metrics at the end of execution
void calculate_performance(float *allWTA, int handled_processes_count, int totalRunTime) //scheduler.perf file
{
    float avgWTA = 0;
    float cpuUtil = ((totalRunTime-idle_time)*100/(float)totalRunTime);

    // Compute average Weighted Turnaround Time (WTA)
    for (int i = 0; i < handled_processes_count; i++)
    { avgWTA += allWTA[i]; }
    avgWTA /= handled_processes_count;

    // Log performance metrics
    fprintf(perfLog, "CPU utilization = %.2f%%\n", cpuUtil);
    fprintf(perfLog, "Avg WTA = %.2f\n", avgWTA);
    fprintf(perfLog, "Avg Waiting = %.2f%%\n", cpuUtil);
}

// Log an event with relevant details
void log_event(const char *event, Process *process)  //scheduler.log file
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

// matensoosh t.update.o el finish time w el hagat di pls
void handle_HPF() 
{

}

void handle_SJF()
{

}

void handle_MLFQ() 
{

}

void handle_RR() 
{

}