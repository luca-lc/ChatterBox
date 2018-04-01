/** $queue_c$ **/

/**
 * \file queue.c
 * \section LICENSE
 * ****************************************************************************
 * \author Luca Canessa (516639)									 		  *
 *																			  *
 * \copyright \n															  *
 * Declares that all contents of this file are author's original operas		  *
 * 																			  *
 * ****************************************************************************
 * \section DESCRIPTION
 * Looking overview
 *
 * This file contains functions used to manage queue structure. 'queue_t' is the 
 * type of double linked FIFO queue. This structure contains a pointer to head 
 * and another one to end of queue, both of type 'node_t', a counter of nodes 
 * and a mutex variable used to syncronizing the accesses.
 *
 * initialQueue() : Allocates space for new queue of type queue_t and 
 * 					initializes its items.
 * 					Returns pointer to new queue
 *
 * push() 		  : Creates a new node, fills its items, put node in the queue
 * 					(ANY OPERATIONS INSIDE MUST BE SAFE WITH MUTEX ACQUIRED)
 * 					Updates also the number of nodes inside the queue.
 * 					Requires pointer to queue where put the node and pointer to
 * 					element to add in the node.
 * 					Returns 0 on success, -1 otherwise
 *
 * pull()         :	Extracts the head node from queue and returns the
 *					pointer to element inside it.
 *					(ANY OPERATIONS INSIDE MUST BE SAFE WITH MUTEX ACQUIRED)
 *					Updates also the number of nodes inside the queue.
 *                  Requires pointer to queue
 *                  Returns the 'void *' pointer to the element extracted
 * 
 * remove_node()  : Removes a particular node from queue BUT DOES NOT 
 *                 	return the pointer to the element contained in this 
 *                  node.
 * 					(ANY OPERATIONS INSIDE MUST BE SAFE WITH MUTEX ACQUIRED)
 * 					Update also the queue lenth.
 *                  Requires the pointer to queue and pointer to the node 
 *                  to be removed.
 *                  Returns 1 if successfully terminate, 0 otherwise.
 * 
 * clear_queue()  : Removes all nodes presents in queue using 'pull()' 
 *                  function. This function uses 'free()' function on 
 *                  pointer returned from 'pull()'.
 *                  (MAKE ATTENTION TO MEMORY LEAK)
 *                  Requires pointer to queue to be cleaned.
 *                  Returns nothing.
 * 
 * destroy_queue(): Cleans queue and frees memory using 'clear_queue()'.
 *                  (Read 'clear_queue()' description for notes).
 *                  Requires pointer to queue to be destroyed.
 *                  Returns nothing.
 */



/******************************************************************************
                                    HEADER
******************************************************************************/
#include <src/queue.h>



/******************************************************************************
									FUNCTIONS
******************************************************************************/
/**
 * \fn			initialQueue
 * \brief       Allocates space for new queue, initializes its items.
 * 				(allocates space also for first node to check later if element 
 * 				pointer exists)
 * \return  q   Pointer to new queue if the space was allocated, NULL otherwise
 */
queue_t *initialQueue()
{
    queue_t *q = NULL;

    if( (q = ( queue_t * )calloc(1, sizeof( queue_t ) )) == NULL ) //check if malloc is ok
    {
        fprintf( stderr, "Problem to allocates space for queue" );
        return NULL;
    }

    q->queue_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    if( (q->head = ( node_t * )calloc( 1, sizeof( node_t ) )) == NULL )
    {
    	fprintf( stderr, "Problem to allocates space for queue" );
    	free( q );
		return NULL;
    }

    q->head->ptr = NULL;
    q->head->next = NULL;
    q->head->prev = NULL;
    q->tail = q->head;
    q->queue_len = 0;

    return q;
}



/**
 * \fn					push
 * \brief				If insert the first element in queue, sets items of the 
 * 						existing node, else creates a new node and put it at 
 * 						end of the queue. In any case updates counter of nodes
 * 						in queue.
 * 						All operations MUST be doing in safe with mutual 
 * 						exclusion variables acquired
 * \param   q        	Pointer to the queue where add data
 * \param   new_data 	Pointer to data to add
 * \return				0 if terminates without errors, -1 otherwise.
 */
int push( queue_t *q, void *new_data )
{
	pthread_mutex_lock( &q->queue_lock );
		if( q->tail->ptr == NULL )
		{
				q->tail->ptr = new_data;
				q->tail->next = NULL;
				q->tail->prev = NULL;
				q->queue_len += 1;
		}
		else
		{
			node_t *newn = NULL;
			if( (newn = ( node_t * )malloc( sizeof( struct node ) )) == NULL )
			{
				pthread_mutex_unlock( &q->queue_lock );
				return -1;
			}

			newn->ptr = new_data;
			newn->next = NULL;

				q->tail->next = newn;
				q->tail->next->prev = q->tail;
				q->tail = newn;
				q->queue_len += 1;
		}
	pthread_mutex_unlock( &q->queue_lock );

	return 0;
}



/**
 * \fn				pull
 * \brief			Removes the node from head of queue and returns its element
 * 					If the node to be removed is the only one, completely free 
 * 					the node and relocate it to return to the basic situation
 *					All operations MUST be doing in safe with mutual 
 * 					exclusion variables acquired
 * \param	q		Pointer to queue
 * \return	ret		(void *) pointer to element inside the node
 */
void *pull( queue_t *q )
{
	void *ret = NULL;
	pthread_mutex_lock( &q->queue_lock );
		if( q != NULL && q->head != NULL )
		{
			if( q->head->ptr != NULL )
			{
				node_t *tmp = NULL;
				if( q->head->next == NULL )
				{
					tmp = q->head;

					q->head = ( node_t * )malloc( sizeof( node_t ) );
					q->head->ptr = NULL;
					q->head->next = NULL;
					q->head->prev = NULL;
					q->tail = q->head;
					q->queue_len = 0;
				}
				else
				{
					tmp = q->head;

					q->head = q->head->next;
					q->head->prev = NULL;
					q->queue_len -= 1;
				}

				ret = tmp->ptr;
				free( tmp );
			}
		}
	pthread_mutex_unlock( &q->queue_lock );

	return ret;
}



/**
 * \fn				remove_node
 * \brief			Removes from queue the node pointed by rm. If the node to 
 * 					be removed is the only one, completely free the node and 
 * 					relocate it to return to the basic situation.
 *					All operations MUST be doing in safe with mutual 
 * 					exclusion variables acquired.
 * \param	q		Pointer to queue
 * \param	rm		Pointer to the node that needs to be removed 
 * \return			1 if exit without errors, 0 otherwise
 */
int remove_node( queue_t *q, node_t *rm )
{
	pthread_mutex_lock( &q->queue_lock );

		if( rm != NULL && rm->ptr != NULL )
		{
			if( rm->prev == NULL )
			{
				if( rm->next == NULL )
				{
						if( rm )
							free( rm );

						q->head = ( node_t * )calloc(1, sizeof( node_t ) );
						q->head->ptr = NULL;
						q->head->next = NULL;
						q->head->prev = NULL;
						q->tail = q->head;
						q->queue_len = 0;
				}
				else
				{
						q->head = q->head->next;
						q->head->prev = NULL;
						q->queue_len -= 1;
						free( rm );
				}
			}
			else
			{
				if( rm->next == NULL )
				{
						q->tail = q->tail->prev;
						q->tail->next = NULL;
						q->queue_len -= 1;
						free( rm );
				}
				else
				{
						rm->prev->next = rm->next;
						rm->next->prev = rm->prev;
						q->queue_len -= 1;
						free( rm );
				}
			}
	pthread_mutex_unlock( &q->queue_lock );

		return 1;
		}
		else
		{
	pthread_mutex_unlock( &q->queue_lock );

		return 0;
		}
}



/**
 * \fn				clear_queue
 * \brief			Removes all nodes from queue and frees pointers returned from 'pull()'
 * \param	 q		Pointer to the queue to be cleaned
 * \bug				Erases all node but not the last
 */
void clear_queue( queue_t *q )
{
	while( q->head->ptr != NULL )
	{
		void *p = pull( q );
		if( p != NULL )
			free( p );
	}
}



/**
 * \fn				destroy_queue
 * \brief       	Removes all nodes from the queue and deletes the queue
 * 					using 'clear_queue()'
 * \param   q   	Pointer to queue to clean.
 */
void destroy_queue( queue_t *q )
{
	if( q != NULL )
	{
		clear_queue( q );

		pthread_mutex_lock( &q->queue_lock );
			if( q->head->ptr != NULL )
				free( q->head->ptr );
			free( q->head );
		pthread_mutex_unlock( &q->queue_lock );

		free( q );
	}
}

