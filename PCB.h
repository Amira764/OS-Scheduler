#include "headers.h"
#include "ProcessQueue.h"
#include <stdio.h>
#include <stdlib.h>
#pragma once

// PCB Table Variables
Process *PCB_Table = NULL;  // Pointer to the PCB table
int PCB_Size = 0;           // Number of processes in the PCB table
int PCB_Capacity = 0;       // Current allocated capacity of the PCB table

// Function to initialize the PCB table
void init_PCB()
{
    PCB_Table = NULL;
    PCB_Size = 0;
    PCB_Capacity = 0;
}

// Function to add a process to the PCB table
void add_to_PCB(Process *process)
{
    if (PCB_Size == PCB_Capacity)
    {
        PCB_Capacity = (PCB_Capacity == 0) ? 1 : PCB_Capacity * 2;
        PCB_Table = realloc(PCB_Table, PCB_Capacity * sizeof(Process));
        if (!PCB_Table)
        {
            perror("Failed to allocate memory for PCB table");
            exit(EXIT_FAILURE);
        }
    }
    PCB_Table[PCB_Size] = *process;
    PCB_Size++;
}

// Function to remove a process from the PCB table by its PID
void remove_from_PCB(int pid)
{
    int found = 0;
    for (int i = 0; i < PCB_Size; i++)
    {
        if (PCB_Table[i].pid == pid)
        {
            found = 1;
            for (int j = i; j < PCB_Size - 1; j++)
            {
                PCB_Table[j] = PCB_Table[j + 1];
            }
            PCB_Size--;
            break;
        }
    }
    if (!found)
    {
        fprintf(stderr, "Process with PID %d not found in PCB table\n", pid);
    }
}

// Function to free the PCB table
void free_PCB()
{
    free(PCB_Table);
    PCB_Table = NULL;
    PCB_Size = 0;
    PCB_Capacity = 0;
}

// Optional: Function to print the PCB table for debugging
void print_PCB()
{
    printf("PCB Table:\n");
    printf("PID\tArrival\tRuntime\tRemaining\tState\n");
    for (int i = 0; i < PCB_Size; i++)
    {
        printf("%d\t%d\t%d\t%d\t\t%d\n",
               PCB_Table[i].pid,
               PCB_Table[i].arrivaltime,
               PCB_Table[i].runtime,
               PCB_Table[i].remainingtime,
               PCB_Table[i].state);
    }
}
