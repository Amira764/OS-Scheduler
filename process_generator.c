#include "headers.h"
#include <string.h>
#include "ProcessQueue.h"

void clearResources(int);

int clk_pid;
int scheduler_pid;
int qid;
void parseArguments(int argc, char *argv[], char **inputFile, int *scheduling_algorithm, int *quantum);
#define buffersize 100
struct msgbuff
{
    long mtype;
    Process mtext;
};

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // Variables to store parsed values
    char *inputFileName = NULL;
    int scheduling_algorithm = -1;
    int quantum = 0;

    // Parse the command-line arguments
    parseArguments(argc, argv, &inputFileName, &scheduling_algorithm, &quantum);

    // TODO Initialization
    // 1. Read the input files.
    char buffer[buffersize];
    FILE *InputFile;
    InputFile = fopen(inputFileName, "r");
    if (InputFile < 0)
    {
        printf(">> Could not open the file\n");
        return -1;
    }
    // Pre-scan the file to count the number of processes (excluding comments)
    int Nprocesses = 0;
    while (fgets(buffer, buffersize, InputFile) != NULL)
    {
        if (buffer[0] == '#') // Skip comments
            continue;
        Nprocesses++;
    }
    // Rewind the file pointer to read the data again
    rewind(InputFile);
    ProcessQueue Processes;
    init_ProcessQueue(&Processes, Nprocesses);
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
        sprintf(RRslice, "%d", quantum);
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
        Process * process_in_turn = peek_ProcessQueue(&Processes);
        if (x == process_in_turn->arrivaltime)
        {
            process_in_turn = dequeue_ProcessQueue(&Processes);
            message.mtext = *process_in_turn;
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


// Function to parse command-line arguments
void parseArguments(int argc, char *argv[], char **inputFile, int *scheduling_algorithm, int *quantum)
{
    if (argc < 2)
    {
        printf("Usage: ./process_generator.o <input_file> -sch <algorithm> [-q <quantum>]\n");
        exit(1);
    }

    // Parse arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-sch") == 0 && (i + 1) < argc)
        {
            *scheduling_algorithm = atoi(argv[i + 1]); // Scheduling algorithm
            i++; // Skip the next argument as it's already processed
        }
        else if (strcmp(argv[i], "-q") == 0 && (i + 1) < argc)
        {
            *quantum = atoi(argv[i + 1]); // Quantum time for RR
            i++; // Skip the next argument
        }
        else if (*inputFile == NULL)
        {
            *inputFile = argv[i]; // Input file name
        }
        else
        {
            printf("Unknown argument: %s\n", argv[i]);
            exit(1);
        }
    }

    // Error checks
    if (*inputFile == NULL)
    {
        printf("Error: Input file not specified.\n");
        exit(1);
    }
    if (*scheduling_algorithm == -1)
    {
        printf("Error: Scheduling algorithm not specified. Use -sch <algorithm>.\n");
        exit(1);
    }
}

