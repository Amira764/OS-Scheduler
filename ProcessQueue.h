#include "headers.h"
#pragma once

// struct representing a process
typedef struct
{
    int id;
    int pid; // when executing execv
    int arrivaltime;
    int runtime;
    int priority;
    int remainingtime;
    int waitingtime;
    int finishtime;
    int TA;
    float WTA;
    int state; // 0 for running , 1 for waiting
} Process;

#define MAX_QUEUE_SIZE 100

// Structure representing a queue of processes
typedef struct
{
    Process items[MAX_QUEUE_SIZE];
    int front;
    int rear;
} ProcessQueue;

// Function to initialize a process queue
void init_ProcessQueue(ProcessQueue *queue)
{
    queue->front = -1;
    queue->rear = -1;
}

// Function to check if the process queue is empty
int isEmpty_ProcessQueue(ProcessQueue *queue)
{
    return (queue->front == -1 && queue->rear == -1);
}

// Function to check if the process queue is full
int isFull_ProcessQueue(ProcessQueue *queue)
{
    return (queue->rear + 1) % MAX_QUEUE_SIZE == queue->front;
}

// Function to enqueue a process
void enqueue_ProcessQueue(ProcessQueue *queue, Process process)
{
    if (isFull_ProcessQueue(queue))
    {
        printf("Process queue is full. Cannot enqueue.\n");
        return;
    }

    if (isEmpty_ProcessQueue(queue))
    {
        queue->front = 0;
        queue->rear = 0;
    }
    else
    {
        queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    }

    queue->items[queue->rear] = process;
}

// Function to dequeue a process
Process dequeue_ProcessQueue(ProcessQueue *queue)
{
    Process emptyProcess = {.id = -1, .arrivaltime = -1, .runtime = -1, .priority = -1};

    if (isEmpty_ProcessQueue(queue))
    {
        printf("Process queue is empty. Cannot dequeue.\n");
        return emptyProcess;
    }

    Process dequeuedProcess = queue->items[queue->front];
    if (queue->front == queue->rear)
    {
        // Last element in the queue
        queue->front = -1;
        queue->rear = -1;
    }
    else
    {
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    }

    return dequeuedProcess;
}

// Function to peek at the front process of the queue
Process peek_ProcessQueue(ProcessQueue *queue)
{
    Process emptyProcess = {.id = -1, .arrivaltime = -1, .runtime = -1, .priority = -1};

    if (isEmpty_ProcessQueue(queue))
    {
        printf("Process queue is empty. Cannot peek.\n");
        return emptyProcess;
    }

    return queue->items[queue->front];
}