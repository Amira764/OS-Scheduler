#include <stdio.h>
#include <stdlib.h>
#include "ProcessQueue.h"  // Assuming this file defines the Process structure

#define NUM_LEVELS 11 

// Define the linked list node structure
typedef struct Node {
    Process process;
    struct Node *next;
} Node;

// Enqueue function for adding a process to the end of a linked list at a given level
void enqueue_linkedlist(Node **level, Process process) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed!\n");
        return;
    }
    new_node->process = process;
    new_node->next = NULL;

    // If the level is empty, the new node becomes the first node
    if (*level == NULL) {
        printf("inserted first node \n");
        *level = new_node;
    } else {
        // Traverse to the end of the list and append the new node (TODO: FOR OPTIMIZATION KEEP TRACK OF LAST B3DEN)
        Node *temp = *level;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
        printf("inserted a node (not first) \n");
    }
}

// Dequeue function to remove and return the first process in the list for a given level
Process* dequeue_linkedlist(Node **level) {
    if (*level == NULL) {
        return NULL;  // Return NULL if the list is empty
    }

    // Remove the head node from the linked list
    Node *temp = *level;
    *level = temp->next;

    Process *process = &temp->process;  // Get the process from the dequeued node
    free(temp);  // Free the memory of the dequeued node
    return process;
}

// Function to check if a given level is empty
int isLevelEmpty(Node *level) {
    return level == NULL;
}

// Function to get the size of the linked list at a specific level
int getLevelSize(Node *level) {
    int size = 0;
    Node *current = level;
    while (current != NULL) {
        size++;
        current = current->next;
    }
    return size;
}

// Function to check if all levels are empty (except level 10)
int allLevelsEmptyExceptLevel10(Node **levels) {
    for (int i = 0; i < NUM_LEVELS - 1; i++) {
        if (levels[i] != NULL) {
            return 0;  // Not empty
        }
    }
    return 1;  // All levels except 10 are empty
}

// Function to check if all levels are empty
int allLevelsEmpty(Node **levels) {
    for (int i = 0; i < NUM_LEVELS; i++) {
        if (levels[i] != NULL) {
            return 0;  // Not empty
        }
    }
    return 1;  // All levels are empty
}

// Function to find the next non-empty level from the given range
int findNextNonEmptyLevel(Node **levels, int start, int end) {
    for (int i = start; i <= end; i++) {
        if (levels[i] != NULL) {
            return i;  // Return the first non-empty level
        }
    }
    return -1;  // Return -1 if no non-empty level is found
}
