#include <stdio.h>
#include <stdlib.h>
#include "ProcessQueue.h" // Assuming this file defines the Process structure

#define MAX_QUEUE_SIZE 30 // Maximum size of the priority queue

typedef struct PriorityQueue
{
    Process *processes; // Use a pointer for dynamic array
    int size;           // Current number of processes in the queue
    int capacity;       // Maximum capacity of the queue
} PriorityQueue;

// Function to initialize the priority queue with dynamic size
void initPQ(PriorityQueue *pq, int Nprocess)
{
    pq->processes = (Process *)malloc(Nprocess * sizeof(Process)); // Dynamically allocate memory for processes
    if (!pq->processes)
    {
        printf("Memory allocation failed!\n");
        exit(EXIT_FAILURE); // Exit if memory allocation fails
    }
    pq->size = 0;            // Initialize the queue size to 0
    pq->capacity = Nprocess; // Set the capacity of the queue
}

// Function to enqueue a process into the priority queue
void enqueue_PQ(PriorityQueue *pq, Process process, bool priority)
{
    if (pq->size >= pq->capacity)
    {
        printf("Queue is full, cannot enqueue process!\n");
        return;
    }

    // Insert process in sorted order (lower number = higher priority)
    int i = pq->size - 1;

    // Find the correct position by shifting processes with higher priority
    while (priority && i >= 0 && pq->processes[i].priority > process.priority)
    {
        pq->processes[i + 1] = pq->processes[i];
        i--;
    }
    while (!priority && i >= 0 && pq->processes[i].remainingtime > process.remainingtime)
    {
        pq->processes[i + 1] = pq->processes[i];
        i--;
    }

    pq->processes[i + 1] = process; // Insert the new process
    pq->size++;
}

// Function to dequeue the process with the highest priority (front of the queue)
Process *dequeue_PQ(PriorityQueue *pq)
{
    if (pq->size == 0)
    {
        return NULL; // Return NULL if the queue is empty
    }

    // Allocate memory to return the dequeued process
    Process *highest_priority_process = (Process *)malloc(sizeof(Process));
    if (!highest_priority_process)
    {
        printf("Memory allocation failed in dequeue!\n");
        exit(EXIT_FAILURE);
    }

    // Copy the highest-priority process (at index 0)
    *highest_priority_process = pq->processes[0];

    // Shift all remaining processes down
    for (int i = 1; i < pq->size; i++)
    {
        pq->processes[i - 1] = pq->processes[i];
    }

    pq->size--;                      // Decrease the queue size
    return highest_priority_process; // Return the pointer to the dequeued process
}

// Function to check if the queue is empty
int isEmpty_PQ(PriorityQueue *pq)
{
    return pq->size == 0;
}

// Function to peek at the process with the highest priority without removing it
// Function to peek at the process with the highest priority without removing it
Process *peek_PQ(PriorityQueue *pq)
{
    if (pq->size == 0)
    {
        return NULL; // Return NULL if the queue is empty
    }

    // Return the pointer to the first process (highest priority)
    return &(pq->processes[0]);
}

// Function to free the dynamically allocated memory for the queue
void freePQ(PriorityQueue *pq)
{
    if (pq->processes)
    {
        free(pq->processes); // Free the dynamically allocated memory
        pq->processes = NULL;
    }
    pq->size = 0;     // Reset the size to 0
    pq->capacity = 0; // Reset the capacity to 0
}