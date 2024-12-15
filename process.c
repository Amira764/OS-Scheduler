#include "headers.h"
#include <signal.h>
#include <stdbool.h>

/* In case of SJF, this will run until remainingtime=0 and argv[1] is the runtime of the process
    Case of RR or Multilevel feedback queue, argv[1] is the quantum, it will run until remainingtime=0, PCB updated in scheduler
    Case of HPF preemptive argv[1] is the runtime of the process, will run until preepted and return remaining time
*/ // Outdated comment

int remainingtime;

int main(int argc, char *argv[])
{
    // Initialize the clock
    initClk();

    // Parse arguments
    remainingtime = atoi(argv[1]); // Time for which the process should run

    while (remainingtime > 0)
    {
        remainingtime--; // Decrement the remaining time
    }

    // Clean up and exit
    destroyClk(false);
    exit(0); //Terminate Myself
}
