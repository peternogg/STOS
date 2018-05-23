#pragma once

//*************************************************
// queue.h
//
// Queue implementation
// Limitations: Queue is implemented as a fixed-length array. The array size
// is specified when the array is created. The array cannot grow beyond this 
// size.
//
// Thread safety: None
//
// Dynamic memory use: Q_Init uses mymalloc to get space for the queue
// structure. Since this is a fixed length queue, Q_Enqueue and Q_Dequeue do 
// not perform any dynamic memory operations. Q_Destroy frees the memory 
// acquired by Q_Init.
//
// Author: Philip Howard
//
//

typedef void * queue_t;

//***********************************************
// Q_Init()
// Initializes a queue. 
//
// Return value: queue object that gets passed to all other queue functions
// Returns zero (NULL) on failure.
//
queue_t Q_Init(int size);

//**********************************************
// Q_Destroy
// Frees memory consumed by the queue.
// No queue functions should be called on the queue after the queue is]
// destroyed.
//
// Return value: zero on success, non-zero on failure
//
int Q_Destroy(queue_t q);

//*********************************************
// Q_Enqueue
// Adds an element to the queue.
//
// Return: zero on success, non-zero on failure
//
void * Q_Enqueue(queue_t q, void *data);

//*********************************************
// Q_Dequeue
// Removes an item from the queue and returns it.
//
// Return: pointer to item on success, 0 (NULL) on failure or if the queue
// is empty
//
void * Q_Dequeue(queue_t q);

//*********************************************
// Q_Elements
//
// Returns the number of elements currently in the queue
int Q_Elements(queue_t q);

