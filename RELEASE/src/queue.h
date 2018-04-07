/** $queue_h$ **/

/**
 * @file queue.h
 * @section LICENSE
 * ****************************************************************************
 * @author Luca Canessa (516639)                                              *
 *                                                                            *
 * @copyright \n                                                              *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * @section DESCRIPTION
 * Looking overview
 * 
 * This file contains: the data structures that create the queues and the 
 * prototypes of the functions used to manage this structures. 'queue_t' is the 
 * type of double linked FIFO queue. This structure contains a pointer to head 
 * and another one to end of queue, both of type 'node_t', a counter of nodes 
 * and a mutex variable used to syncronizing the accesses.
 * 
 * initialQueue()   :   Allocates space to create a new queue and initializes 
 *                      its items.
 *                      Does not require input arguments
 *                      Returns pointer to new queue
 * 
 * push()           :   Creates a new node and puts it at the end of queue
 *                      Requires pointer to queue where put the new item and
 *                      pointer to the datus to insert
 *                      Returns 0 if terminates with success, -1 otherwise
 * 
 * pull()           :   Extracts the head element from queue and returns the
 *                      pointer to this element.
 *                      Requires pointer to queue
 *                      Returns the 'void *' pointer to the element extracted
 * 
 * remove_node()    :   Removes a particular node from queue BUT DOES NOT 
 *                      return the pointer to the element contained in this 
 *                      node.
 *                      Requires the pointer to queue and pointer to the node 
 *                      to be removed.
 *                      Returns 1 if successfully terminate, 0 otherwise.
 * 
 * clear_queue()    :   Removes all nodes presents in queue using 'pull()' 
 *                      function. This function uses 'free()' on node pointer.
 *                      (MAKE ATTENTION TO MEMORY LEAK)
 *                      Requires pointer to queue to be cleaned.
 *                      Returns nothing.
 * 
 * destroy_queue()  :   Cleans queue and delete it.
 *                      (Read 'clear_queue()' description for memory leak notes).
 *                      Requires pointer to queue to be destroyed.
 *                      Returns nothing.
 */


/******************************************************************************
                                    HEADER
******************************************************************************/
#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <pthread.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>



/******************************************************************************
                                 TYPES AND STRUCTS
******************************************************************************/
/**
 * @typedef         node_t
 * @struct          struct node
 * @brief           Defines the data structure for queue's nodes
 * @var     ptr     Pointer to element to add to queue
 * @var     next    Pointer to next node of queue (can be NULL)
 * @var     prev    Pointer to previous node of queue (can be NULL)
 */
typedef struct node
{
    void *ptr;
    struct node *next;
    struct node *prev;
}node_t;


/**
 * @typedef             queue_t
 * @struct              struct queue
 * @brief               Define the data structure to implement queue
 * @var     head        Pointer to the first node of the queue
 * @var     tail        Pointer to the last node of the queue
 * @var     queue_len   Counter of nodes present in queue
 * @var     queue_lock  Mutex variable used to synchronize accesses
 */
typedef struct queue
{
    node_t *head;
    node_t *tail;
    unsigned long queue_len;
    pthread_mutex_t queue_lock;
}queue_t;



/******************************************************************************
                                    FUNCTIONS
******************************************************************************/
/**
 * @function          initialQueue
 * @brief       Allocates space for new queue and initialize its elements
 * @return      Pointer to the created queue
 */
queue_t *initialQueue();


/**
 * @function                  push
 * @brief               Creates a new node, fills its items with new data and put node in queue.
 * @param   q           Pointer to queue where insert the node.
 * @param   new_data    Pointer to data to add.
 * @return              0 if function exit without errors, -1 otherwise.
 */
int push( queue_t *q, void *new_data );



/**
 * @function          pull    
 * @brief       Extracts the first element from queue and returns it.
 * @param   q   Pointer to the queue where extract the element.
 * @return	 	Returns pointer to element located in the node.
 */
void *pull( queue_t *q );



/**
 * @function          remove_node
 * @brief       Removes from queue the node passed as parameter.
 * @warning     !!! WARNING: this function remove node but NOT frees the element located in this node !!!
 * @param   q   Pointer to the queue where remove the queue.
 * @param   rm  Pointer to node to remove.
 * @return      1 if node has been removed, 0 otherwise
 */
int remove_node( queue_t *q, node_t *rm );



/**
 * @function          clear_queue
 * @brief		Cleans queue removing all nodes
 * @warning     !!! WARNING: this function remove all nodes in the queue and frees the element in nodes \n
 *              CAUTION to memory leak !!!
 * @param   q	Pointer to the queue to be cleaned
 */
void clear_queue( queue_t *q );



/**
 * @function          destroy_queue
 * @brief       Clears all data from queue removing nodes and frees queue
 * @warning     !!! WARNING: this function remove all nodes in the queue and frees the element in nodes \n
 *              CAUTION to memory leak !!!
 * @param   q   pointer to queue to clean
 */
void destroy_queue( queue_t *q );



#endif //QUEUE_T_H
