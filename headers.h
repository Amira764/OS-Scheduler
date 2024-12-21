#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#pragma once

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
#define SEMKEY 400

// Semaphore union for initialization
union Semun {
    int val;                 // Value for SETVAL
    struct semid_ds *buf;    // Buffer for IPC_STAT, IPC_SET
    unsigned short *array;   // Array for GETALL, SETALL
};

// Create and initialize the semaphore
int createSemaphore() 
{
    int semid = semget(SEMKEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("Failed to create semaphore");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphore to 1 (unlocked state)
    union Semun semun;
    semun.val = 1; // Initial value
    if (semctl(semid, 0, SETVAL, semun) == -1) {
        perror("Failed to initialize semaphore");
        exit(EXIT_FAILURE);
    }

    return semid;
}

// Wait (P operation)
void semaphoreWait(int semid) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1; // Decrement semaphore value
    sem_op.sem_flg = 0;
    if (semop(semid, &sem_op, 1) == -1) {
        perror("Semaphore wait failed");
        exit(EXIT_FAILURE);
    }
}

// Signal (V operation)
void semaphoreSignal(int semid) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = 1; // Increment semaphore value
    sem_op.sem_flg = 0;
    if (semop(semid, &sem_op, 1) == -1) {
        perror("Semaphore signal failed");
        exit(EXIT_FAILURE);
    }
}


///==============================
//don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All processes call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All processes call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
