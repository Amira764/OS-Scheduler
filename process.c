#include "headers.h"
#include <signal.h>
#include <stdbool.h>

/* In case of SJF, this will run until remainingtime=0 and argv[1] is the runtime of the process
    Case of RR or Multilevel feedback queue, argv[1] is the quantum, it will run until remainingtime=0, PCB updated in scheduler
    Case of HPF preemptive argv[1] is the runtime of the process, will run until preepted and return remaining time
*/

int remainingtime;
bool preempted = false;

void handle_preemption(int signum)
{
    preempted = true; // Set preempted flag when the scheduler sends a signal
}

struct {
    int pid;
    int remainingtime;
} message;

int main(int argc, char *argv[])
{
    // Initialize the clock
    initClk();

    // Parse arguments
    remainingtime = atoi(argv[1]); // Time for which the process should run
    int prevClk = getClk();

    // Set up signal handler for preemption
    signal(SIGUSR1, handle_preemption);

    // IPC setup (message queue to notify the scheduler)
    key_t key_id = ftok("keyfile", 60);
    int qid = msgget(key_id, 0666 | IPC_CREAT);

    while (remainingtime > 0)
    {
        if (preempted)
        {
            // Send remaining time to scheduler and exit for preemption
            message.pid = getpid();
            message.remainingtime = remainingtime;
            msgsnd(qid, &message, sizeof(message), IPC_NOWAIT);
            printf("Process with PID %d preempted. Remaining time: %d\n", getpid(), remainingtime);
            destroyClk(false);
            return 0;
        }

        int currentClk = getClk();
        if (currentClk > prevClk)
        {
            remainingtime--; // Decrement the remaining time
            prevClk = currentClk;
        }
    }

    // Notify scheduler of process completion
    message.pid = getpid();
    message.remainingtime = 0; // Indicate process is finished
    msgsnd(qid, &message, sizeof(message), IPC_NOWAIT);
    printf("Process with PID %d finished execution.\n", getpid());

    // Clean up and exit
    destroyClk(false);
    exit(0); //Terminate Myself
}
