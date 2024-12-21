#include "headers.h"
#include <stdbool.h>
#pragma once

typedef struct Mem_Block 
{
    int start;                
    int end;                  
    int size;                 
    int process_id;          
    struct Mem_Block *left; 
    struct Mem_Block *right; 
    struct Mem_Block *parent;
} Mem_Block;

Mem_Block *createMemBlock(int size, int start, int process_id, Mem_Block *parent) 
{
    Mem_Block *block = (Mem_Block *)malloc(sizeof(Mem_Block));
    block->size = size;                  
    block->start = start;                
    block->end = start + size - 1;       
    block->process_id = process_id;      
    block->left = NULL;                  
    block->right = NULL;
    block->parent = parent;              
    return block;
}

Mem_Block *findAvailableBlock(Mem_Block *node, int size) 
{
    if (node == NULL) 
        return NULL;

    // Check if this block is free and large enough
    if (node->process_id == -1 && node->size >= size && node->left == NULL && node->right == NULL)
        return node;

    // Recursively check in the left and right children
    Mem_Block *leftBlock = findAvailableBlock(node->left, size);
    if (leftBlock != NULL)
        return leftBlock;

    return findAvailableBlock(node->right, size);
}

Mem_Block *allocateBlock(Mem_Block *node, Process * process) 
{
    int size = process->mem_size;
    if (node == NULL)
        return NULL;

    // Find the smallest free block that fits the size
    Mem_Block *block = findAvailableBlock(node, size);
    if (block == NULL) 
    {
        printf("No available block for size %d\n", size);
        return NULL;
    }

    // Split the block until it matches the requested size
    while (block->size/2 >= size) 
    {
        int halfSize = block->size / 2;

        // Create left and right children
        block->left = createMemBlock(halfSize, block->start, -1, block);
        block->right = createMemBlock(halfSize, block->start + halfSize, -1, block);

        // Update the block to point to the left child
        block = block->left;
    }

    // Allocate the block
    block->process_id = process->id;
    process->mem_start = block->start;
    process->mem_end = block->end;
    printf("\033[0;31m");  // Set text color to red
    printf("Allocated block for process %d: Start %d, End %d, Size %d\n", process->id, block->start, block->end, block->size);
    printf("\033[0m");  // Reset text color to default
    return block;
}

void mergeBlocks(Mem_Block *node) 
{
    if (node == NULL || node->left == NULL || node->right == NULL)
        return;

    // Check if both children are free
    if (node->left->process_id == -1 && node->right->process_id == -1) 
    {
        // Free the parent block
        free(node->left);
        free(node->right);
        node->left = NULL;
        node->right = NULL;
        node->process_id = -1; // Mark the parent as free
        printf("\033[0;31m");  // Set text color to red
        printf("Merged block: Start %d, End %d, Size %d\n", node->start, node->end, node->size);
        printf("\033[0m");  // Reset text color to default
    }
}

bool freeBlock(Mem_Block *node, int process_id) 
{
    if (node == NULL)
        return false;

    // Find the block to free
    if (node->process_id == process_id) 
    {
        node->process_id = -1; // Mark the block as free
        printf("\033[0;31m");  // Set text color to red
        printf("Freed block for process %d: Start %d, End %d, Size %d\n", process_id, node->start, node->end, node->size);
        printf("\033[0m");  // Reset text color to default
        // Merge with buddies if possible
        mergeBlocks(node);
        return true;
    }

    // Recursively search in left and right children
    bool freedLeft = freeBlock(node->left, process_id);
    bool freedRight = freeBlock(node->right, process_id);

    // Attempt to merge if either child was freed
    if (freedLeft || freedRight)
        mergeBlocks(node);

    return freedLeft || freedRight;
}

