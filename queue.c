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

#include "queue.h"
#include "mymalloc.h"

typedef struct
{
    int head;
    int tail;
    int max;
    void *data[1];
} q_imp_t;

//***********************************************
// Q_Init()
// Initializes a queue. 
//
// Return value: queue object that gets passed to all other queue functions
// Returns zero (NULL) on failure.
//
queue_t Q_Init(int size)
{
    q_imp_t *queue;
    queue = (q_imp_t*)my_malloc(sizeof(q_imp_t) + (size-1)*sizeof(void*));
    if (queue != (q_imp_t*)0)
    {
        queue->head = 0;
        queue->tail = 0;
        queue->max = size;
    }

    return (queue_t)queue;
}

//**********************************************
// Q_Destroy
// Frees memory consumed by the queue.
// No queue functions should be called on the queue after the queue is]
// destroyed.
//
// Return value: zero on success, non-zero on failure
//
int Q_Destroy(queue_t q)
{
    if (q == 0) return -1;
    
    my_free(q);
    return 0;
}

//*********************************************
// Q_Enqueue
// Adds an element to the queue.
//
// Return: zero on success, non-zero on failure
//
void * Q_Enqueue(queue_t q, void *data)
{
    if (q == 0) return -1;
    q_imp_t *queue;
    queue = (q_imp_t*)q;
    int head;
    head = queue->head;
    head = (head + 1) % queue->max;
    if (head == queue->tail) return -2;
    queue->data[queue->head] = data;
    queue->head = head;

    return 0;
}

//*********************************************
// Q_Dequeue
// Removes an item from the queue and returns it.
//
// Return: pointer to item on success, 0 (NULL) on failure or if the queue
// is empty
//
void * Q_Dequeue(queue_t q)
{
    if (q == 0) return 0;
    q_imp_t *queue = (q_imp_t*)q;
    if (queue->head == queue->tail) return 0;

    void *item = queue->data[queue->tail];
    queue->tail = (queue->tail + 1) % queue->max;

    return item;
}
//*********************************************
// Q_Elements
//
// Returns the number of elements currently in the queue
int Q_Elements(queue_t q)
{
    int num;

    q_imp_t *queue = (q_imp_t*)q;

    num = queue->head - queue->tail;
    if (num < 0) num += queue->max;

    return num;
}

