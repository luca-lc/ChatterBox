//
// CHATTERBOX_QUEUE_H
// Created by Luca Canessa - mat. 516639.
// Declares that all contents of this file are author's original operas
//



#ifndef _QUEUE_H_
#define _QUEUE_H_



/******************************************************************************
                                 TYPES AND STRUCTS
******************************************************************************/
/**
 * @brief    defines a new structure type 'node_t' to implements nodes of queue
 * @var ptr  pointer to added datum
 * @var next pointer to next datum
 */
typedef struct node
{
    void *ptr;
    struct node *next;
}node_t;


/**
 * @brief           defines a new structure type 'queue_t' to implements a queue
 * @var head        pointer to top of queue
 * @var tail        pointer to end of queue
 * @var queue_len   counter of queue elements
 */
typedef struct queue
{
    node_t *head;
    node_t *tail;
    unsigned long queue_len;
}queue_t;



/******************************************************************************
                                    FUNCTIONS
******************************************************************************/
/**
 * @brief       copy the number of queue elements in a var and returns its
 * @param   q   pointer to queue structs
 * @return      queue size
 */
unsigned long queue_lenght( queue_t *q );


/**
 * @brief       allocates space for new queue
 * @return      pointer to new queue
 */
queue_t *initialQueue( );


/**
 * @brief            creates a new node and fill its items with new data
 * @param   q        pointer to queue where add data
 * @praam   new_data pointer to new data
 */
int push( queue_t *q, void *new_data );


/**
 * @brief       cleans all data from queue and erase its
 * @param   q   pointer to queue to clean
 */
void clean_all( queue_t *q );



#endif //QUEUE_T_H
