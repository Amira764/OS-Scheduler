#include "headers.h"

int remainingtime;

void decrementTime(int signum)
{
    remainingtime--;
    signal(SIGILL, decrementTime);
}

int main(int agrc, char * argv[])
{
    remainingtime = atoi(argv[1]);
    signal(SIGILL, decrementTime);
    initClk();
   
    while (remainingtime > 0) { }

    printf("\033[0;35m");
    printf("Scheduler: process with id: %d has finished\n", getpid());
    printf("\033[0m");
    destroyClk(false);
    exit(0);
    return 0;
}