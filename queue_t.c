//
// CHATTERBOX_LIST_T_C
// Created by luca - mat. 516639.
// Declares that all contents of this file are author's original operas
//

/******************************************************************************
                                    HEADER
******************************************************************************/
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

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
}


/*
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
*/
