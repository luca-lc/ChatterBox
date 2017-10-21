/* @$queue_c$ */

/**
 * @file queue.h
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c) 2017 Luca Canessa (516639)									  *
 *																			  *
 * Declares that all contents of this file are author's original operas		  *
 * 																			  *
 * **************************************************************************
 * @section DESCRIPTION
 * Looking overview
 *
 * In this file there are functions to create and manipulate queues
 *
 * initialQueue() : Allocates space for new queue_t queue type with nodes_t nodes
 * 					type. The structure of queue was thought as FIFO type.
 * 					Returns pointer to this queue
 *
 * push() 		  : Creates a new node_t node type and insert in ptr element
 * 					the new info data and in next element the pointer to
 * 					following element.
 * 					Requires pointer to a queue and data to insert
 * 					Returns 0 if terminates without error, -1 otherwise.
 *
 * pull()         :	Extracts the first element in FIFO queue and update the
 * 					pointer to head queue.
 * 					Requires pointer to queue_t queue where extract the element
 * 					Returns pointer to data node if there are elements in queue,
 * 					NULL otherwise.
 *
 * clear_queue()  :	Removes all nodes from queue and restores it.
 * 					Requires pointer to queue to be reset.
 *
 *
 *
 * destroy_queue():	//TODO
 * 					Requires pointer to queue_t queue to be destroyed.
 * 					Removes all elements from queue and destroys queue
 *
 *
 *
 */

/******************************************************************************
                                    HEADER
******************************************************************************/
#include <src/queue.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>



/******************************************************************************
									FUNCTIONS
******************************************************************************/
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;


/**
 * @brief       allocates space for new queue
 * @return  q   pointer to new queue
 */
queue_t *initialQueue()
{
    queue_t *q = NULL;

    if( (q = ( queue_t * )malloc( sizeof( queue_t ) )) == NULL )//check if malloc is ok
    {
        fprintf( stderr, "Problem to allocates space for queue" );
        exit( EXIT_FAILURE );
    }
    q->head = ( node_t * )malloc( sizeof( node_t ) );
    q->head->ptr = NULL;
    q->head->next = NULL;
    q->tail = q->head;
    q->queue_len = 0;

    return q;
}



/**
 * @brief            creates a new node and fill its items with param 'new data' and increases the queue length
 * @param   q        pointer to queue where add data
 * @param   new_data pointer to new data
 * @var 	newn	 new node to add at end of queue
 */
int push( queue_t *q, void *new_data )
{
	if( q->tail->ptr == NULL )
	{
		pthread_mutex_lock( &queue_lock );
		q->tail->ptr = new_data;
		q->tail->next = NULL;
		pthread_mutex_unlock( &queue_lock );
	}
	else
	{
		node_t *newn = NULL;
		if( (newn = ( node_t * )malloc( sizeof( node_t ) )) == NULL )
		{
			return -1;
		}

		newn->ptr = new_data;
		newn->next = NULL;

		pthread_mutex_lock( &queue_lock );
		q->tail->next = newn;
		q->tail = newn;
		pthread_mutex_unlock( &queue_lock );
	}

	pthread_mutex_lock( &queue_lock );
	q->queue_len += 1;
	pthread_mutex_unlock( &queue_lock );

	return 0;
}



/**
 * @brief			extracts from queue the first queue element
 * @param	q		pointer to queue
 * @return	ret		pointer to (void *) element in node_t
 */
void *pull( queue_t *q )
{
	node_t *tmp = NULL;

	tmp = (node_t *)q->head;

	pthread_mutex_lock( &queue_lock );
	q->head = q->head->next;
	q->queue_len -= 1;
	pthread_mutex_unlock( &queue_lock );

	void *ret = tmp->ptr;

	return ret;
}



/**
 * @brief			clears all nodes from queue, updates head, tail and length
 * @param	 q		pointer to queue to be cleaned
 */
void clear_queue( queue_t *q )
{
	while( q->head != q->tail )
	{
		node_t *tmp = (node_t *)q->head;
		q->head = q->head->next;
		free( tmp );
	}

	q->head->ptr = NULL;
	q->head->next = NULL;
	q->tail = q->head;

	q->queue_len = 0;
}

/**
 * @brief       	cleans all data from queue and destroys its
 * @param   q   	pointer to queue to clean.
 * @var		tmp 	temporary pointer to head's element
 */
void destroy_queue( queue_t *q )
{
    while( q->head != q->tail )
    {
        node_t *tmp = (node_t *)q->head;
        q->head = q->head->next;
        free( tmp );
    }

    if( q->head )
    {
        free( (void *)q->head );
    }

    free( q );
}

