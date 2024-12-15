#include <stdio.h>
#include <stdlib.h>
#include "ProcessQueue.h"  // Assuming this file defines the Process structure

#define NUM_LEVELS 11 

typedef struct Node {
    Process process;
    struct Node *next;
} Node;

void print_process(Process process);

void enqueue_linkedlist(Node **level, Process process) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed!\n");
        return;
    }
    new_node->process = process;
    new_node->next = NULL;

    if (*level == NULL) {
        *level = new_node;
    } else {
        Node *temp = *level;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

Process dequeue_linkedlist(Node **level) {
    if (*level == NULL) {
        Process null_process = {0};  // Assuming Process has default values
        return null_process;
    }

    Node *temp = *level;
    *level = temp->next;

    Process process = temp->process;  // Copy the process data
    free(temp);  // Free the node
    return process;
}

int isLevelEmpty(Node *level) {
    return level == NULL;
}

int getLevelSize(Node *level) {
    int size = 0;
    Node *current = level;
    while (current != NULL) {
        size++;
        current = current->next;
    }
    return size;
}

int allLevelsEmptyExceptLevel10(Node **levels) {
    for (int i = 0; i < NUM_LEVELS - 1; i++) {
        if (levels[i] != NULL) {
            return 0;
        }
    }
    return 1;
}

int allLevelsEmpty(Node **levels) {
    for (int i = 0; i < NUM_LEVELS; i++) {
        if (levels[i] != NULL) {
            return 0;
        }
    }
    return 1;
}

int findNextNonEmptyLevel(Node **levels, int start, int end) {
    for (int i = start; i <= end; i++) {
        if (levels[i] != NULL) {
            return i;
        }
    }
    return -1;
}


void print_process(Process process) {
    printf("\033[0;31m");  // Set text color to red
    printf("Process Details:\n");
    printf("  ID: %d\n", process.id);
    printf("  PID: %d\n", process.pid);
    printf("  Priority: %d\n", process.priority);
    printf("  Arrival Time: %d\n", process.arrivaltime);
    printf("  Runtime: %d\n", process.runtime);
    printf("  Remaining Time: %d\n", process.remainingtime);
    printf("  State: %d\n", process.state);
    printf("\033[0m");  // Reset text color to default
    printf("----------------------------\n");
}