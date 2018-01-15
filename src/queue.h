/* $queue_h$ */
/**
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c)2017 Luca Canessa (516639)                                   *
 *                                                                            *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************/



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
    struct node *prev;
}node_t;


/**
 * @struct queue
 * @brief           defines a new structure type 'queue_t' to implements a queue
 * @var head        pointer to top of queue
 * @var tail        pointer to end of queue
 * @var queue_len   counter of queue elements
 */
typedef struct queue
{
    node_t *head; ///< pointer to head of queue
    node_t *tail; ///< pointer to tail of queue
    unsigned long queue_len; ///< variable to save queue lenght
}queue_t;



/******************************************************************************
                                    FUNCTIONS
******************************************************************************/
/**
 * @brief       allocates space for new queue
 * @return      pointer to new queue
 */
queue_t *initialQueue();


/**
 * @brief            creates a new node and fill its items with new data
 * @param   q        pointer to queue where add data
 * @param   new_data pointer to new data
 */
int push( queue_t *q, void *new_data );



/**
 * @brief			extracts from queue the first element and return its info
 * @param	 q		pointer to queue_t queue type
 * @return	 		returns pointer to (void *) element about node
 */
void *pull( queue_t *q );



/**
 * @brief			clears all nodes from queue, updates head, tail and length
 * @param	 q		pointer to queue to be cleaned
 */
void clear_queue( queue_t *q );



/**
 * @brief       cleans all data from queue and erase its
 * @param   q   pointer to queue to clean
 */
void destroy_queue( queue_t *q );



#endif //QUEUE_T_H
