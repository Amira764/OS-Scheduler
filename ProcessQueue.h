#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#pragma once

// Structure representing a process
typedef struct
{
    int id;
    int pid; // Process ID when executing execv
    int arrivaltime;
    int runtime;
    int priority;
    int remainingtime;
    int waitingtime;
    int finishtime;
    int TA;
    float WTA;
    int state; // 0 for running, 1 for waiting
} Process;

// Dynamically adjustable ProcessQueue structure
typedef struct
{
    Process **items; // Array of pointers to Process
    int front;
    int rear;
    int size; // Capacity of the queue
} ProcessQueue;

// Function to initialize a process queue with a given size
void init_ProcessQueue(ProcessQueue *queue, int size)
{
    queue->items = (Process **)malloc(size * sizeof(Process *));
    if (!queue->items)
    {
        perror("Failed to allocate memory for the process queue");
        exit(EXIT_FAILURE);
    }
    queue->front = -1;
    queue->rear = -1;
    queue->size = size;
}

// Function to check if the process queue is empty
bool isEmpty_ProcessQueue(ProcessQueue *queue)
{
    return (queue->front == -1 && queue->rear == -1);
}

// Function to check if the process queue is full
bool isFull_ProcessQueue(ProcessQueue *queue)
{
    return (queue->rear + 1) % queue->size == queue->front;
}

// Function to enqueue a process
void enqueue_ProcessQueue(ProcessQueue *queue, Process process)
{
    if (isFull_ProcessQueue(queue))
    {
        printf("Process queue is full. Cannot enqueue.\n");
        return;
    }

    // Dynamically allocate memory for the process
    Process *new_process = (Process *)malloc(sizeof(Process));
    if (!new_process)
    {
        perror("Failed to allocate memory for the process");
        exit(EXIT_FAILURE);
    }

    // Copy the process data into the newly allocated memory
    *new_process = process;

    // Enqueue the dynamically allocated process
    if (isEmpty_ProcessQueue(queue))
    {
        queue->front = 0;
        queue->rear = 0;
    }
    else
    {
        queue->rear = (queue->rear + 1) % queue->size;
    }

    queue->items[queue->rear] = new_process;
}

// Function to dequeue a process
Process *dequeue_ProcessQueue(ProcessQueue *queue)
{
    if (isEmpty_ProcessQueue(queue))
    {
        printf("Process queue is empty. Cannot dequeue.\n");
        return NULL;
    }

    Process *dequeuedProcess = queue->items[queue->front];
    if (queue->front == queue->rear)
    {
        // Last element in the queue
        queue->front = -1;
        queue->rear = -1;
    }
    else
    {
        queue->front = (queue->front + 1) % queue->size;
    }

    return dequeuedProcess;
}

// Function to peek at the front process of the queue
Process *peek_ProcessQueue(ProcessQueue *queue)
{
    if (isEmpty_ProcessQueue(queue))
    {
        printf("Process queue is empty. Cannot peek.\n");
        return NULL;
    }

    return queue->items[queue->front];
}

// Function to free the dynamically allocated queue
void free_ProcessQueue(ProcessQueue *queue)
{
    // Free each individual process in the queue
    while (!isEmpty_ProcessQueue(queue))
    {
        Process *process = dequeue_ProcessQueue(queue);
        free(process);
    }

    // Free the queue itself
    free(queue->items);
    queue->items = NULL;
    queue->size = 0;
    queue->front = -1;
    queue->rear = -1;
}

// Function to print the contents of the process queue
void print_ProcessQueue(ProcessQueue *queue, int size)
{
    if (isEmpty_ProcessQueue(queue))
    {
        printf("Process queue is empty.\n");
        return;
    }

    printf("Process Queue:\n");
    printf("ID\tPID\tArrival\tRuntime\tPriority\tRemaining\tWaiting\tState\n");

    int i = queue->front;
    do
    {
        Process *process = queue->items[i];
        printf("%d\t%d\t%d\t%d\t%d\t\t%d\t\t%d\t%d\n",
               process->id,
               process->pid,
               process->arrivaltime,
               process->runtime,
               process->priority,
               process->remainingtime,
               process->waitingtime,
               process->state);

        i = (i + 1) % queue->size; // Move to the next process
    } while (i != (queue->rear + 1) % queue->size); // Stop when we've printed all elements
}