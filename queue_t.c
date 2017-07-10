//
// CHATTERBOX_QUEUE_T_C
// Created by Luca Canessa - mat. 516639.
// Declares that all contents of this file are author's original operas
//

/******************************************************************************
                                    HEADER
******************************************************************************/
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>



/******************************************************************************
									FUNCTIONS
******************************************************************************/

/**
 * @brief       allocates space for new queue
 * @return  q   pointer to new queue
 */
queue_t *initialQueue( )
{
    queue_t *q = NULL;

    if( (q = ( queue_t * )malloc( sizeof( queue_t ) )) == NULL )//check if malloc is ok
    {
        fprintf( stderr, "Problem to allocates space for queue" );
        exit( EXIT_FAILURE );
    }
    printf( "puppa!!" );
    q->head = ( node_t * )malloc( sizeof( node_t ) );
    q->head->ptr = NULL;
    q->head->next = NULL;
    q->tail = q->head;
    q->queue_len = 0;

    return q;
}//end initialQueue



/**
 * @brief            creates a new node and fill its items with param 'new data' and increases the queue length
 * @param   q        pointer to queue where add data
 * @praam   new_data pointer to new data
 * @var 	newn	 new node to add at end of queue
 */
int push( queue_t *q, void *new_data )
{
	node_t *newn = NULL;
	if( (newn = ( node_t * )malloc( sizeof( node_t ) )) == NULL )
	{
		return -1;
	}

	newn->ptr = new_data;
	newn->next = NULL;

	q->tail->next = newn;
	q->tail = newn;

	q->queue_len += 1;

	return 0;
}//end push



/**
 * @brief       cleans all data from queue and erase its
 * @param   q   pointer to queue to clean.
 * @var		tmp temporary pointer to head's element
 */
void clean_all( queue_t *q )
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
}//end clean_all

