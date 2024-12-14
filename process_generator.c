#include "headers.h"
#include <string.h>
#include "ProcessQueue.h"

void clearResources(int);

int clk_pid;
int scheduler_pid;
int qid;

#define buffersize 100
struct msgbuff
{
    long mtype;
    Process mtext;
};

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // TODO Initialization
    // 1. Read the input files.
    int Nprocesses = 0;
    ProcessQueue Processes;
    init_ProcessQueue(&Processes);

    char buffer[buffersize];
    FILE *InputFile;
    InputFile = fopen("processes.txt", "r");
    if (InputFile < 0)
    {
        printf(">> Could not open the file\n");
        return -1;
    }
    while (fgets(buffer, buffersize, InputFile) != NULL) // Read the file line by line
    { 
        if (buffer[0] == '#') // comment
        { continue; }
        Process p;
        p.id = atoi(strtok(buffer, "\t")); // Save First token: ID
        p.arrivaltime = atoi(strtok(NULL, "\t")); // Save Second token: arrival time
        p.runtime = atoi(strtok(NULL, "\t")); // Save third token: runtime
        p.priority = atoi(strtok(NULL, "\t")); // Save fourth token: priority
        p.remainingtime = p.runtime;
        enqueue_ProcessQueue(&Processes, p);
        Nprocesses++;
    }

    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    printf("Choose the scheduling algorithm"); 
    printf("Enter :  \n 1. Shortest Job First (SJF) \n 2. Preemptive Highest Priority First (HPF) \n 3. Round Robin (RR) \n 4. Multiple level Feedback Loop >> ");
    
    fgets(buffer, buffersize, stdin);
    int scheduling_algorithm = atoi(buffer);
    int TimeSlice=0;
    if (scheduling_algorithm == 3)
    {
        printf("Enter TimeSlice: ");
        fgets(buffer, buffersize, stdin);
        TimeSlice = atoi(buffer);
    }

    // 3. Initiate and create the scheduler and clock processes.
    clk_pid = fork();
    if (clk_pid == 0)
    { execv("clk.out", argv); }

    scheduler_pid = fork();
    if (scheduler_pid == 0)
    {
        char algorithm[12]; // Sufficient for a 32-bit signed integer
        char N[12];
        char RRslice[12];
        sprintf(algorithm, "%d", scheduling_algorithm);
        sprintf(N, "%d", Nprocesses);
        sprintf(RRslice, "%d", TimeSlice);
        char *args[] = {"./scheduler.out", algorithm, N, RRslice, NULL};
        execv("./scheduler.out", args);
    }

    // 4. Use this function after creating the clock process to initialize clock.
    initClk();
    // To get time use this function. 
    int x = getClk();
    printf("Current Time is %d\n", x);

    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    struct msgbuff message;
    key_t key_id = ftok("keyfile", 70);
    qid = msgget(key_id, 0666 | IPC_CREAT); // message queue to send processes to scheduler file

    // 6. Send the information to the scheduler at the appropriate time.
    while (!isEmpty_ProcessQueue(&Processes))
    {
        x = getClk();
        Process process_in_turn = peek_ProcessQueue(&Processes);
        if (x == process_in_turn.arrivaltime)
        {
            process_in_turn = dequeue_ProcessQueue(&Processes);
            message.mtext = process_in_turn;
            msgsnd(qid, &message, sizeof(message.mtext), IPC_NOWAIT);
            printf("sending %d", message.mtext.id);
        }
    }

    // 7. Clear clock resources
    int status;
    waitpid(scheduler_pid, &status, 0);
    destroyClk(true);

    return 0;
}

 //TODO Clears all resources in case of interruption
void clearResources(int signum)
{
    kill(clk_pid, SIGINT);
    kill(scheduler_pid, SIGINT);
    msgctl(qid, IPC_RMID, NULL); // Remove message queue
    destroyClk(true);
    exit(0);
}


