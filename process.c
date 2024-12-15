#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

void decrementTime(int signum)
{
    remainingtime--;
    printf("Inside decrement \n");
    //printf("Scheduler: process with id: %d is running and  has remaining time %d at TIME: %d \n", getpid(), remainingtime ,getClk());
}

int main(int agrc, char * argv[])
{
    remainingtime = atoi(argv[1]);
    //printf("Scheduler: process with id: %d is ready with remaining time  %d \n", getpid() , remainingtime);
    
    signal(SIGUSR1, decrementTime);

    initClk();
    //remaining time will be passed from the scheduler
   
    //TODO it needs to get the remaining time from somewhere
    while (remainingtime > 0)
    {
        //still running
    }
    //change color to purple
    printf("\033[0;35m");
    printf("Scheduler: process with id: %d has finished\n", getpid());
    printf("\033[0m");
    destroyClk(false);
    exit(0);
    return 0;
}