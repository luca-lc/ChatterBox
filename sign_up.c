//
// CHATTERBOX_SIGN_UP_C
// Created by Luca Canessa - mat. 516639.
// Declares that all contents of this file are author's original operas
//



/******************************************************************************
									HEADER
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <queue.h>
#include <ops.h>
#include <sign_up.h>
#include <stats.h>


static pthread_mutex_t reg_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t reg_cond = PTHREAD_COND_INITIALIZER;



/******************************************************************************
									FUNCTIONS
******************************************************************************/
/**
 * @brief	checks if in the 'reg' list is present the item 'elem'
 * @param	reg		is a pointer to queue of type queue_t
 * @param	elem 	is a pointer to element of type 'void *'
 * @var		tmp		is a temporary pointer to an element of queue
 * @return	a code of a request operation
 */
op_t is_registrated( queue_t *reg, void *elem )
{
	node_t *tmp = ( node_t * )reg->head;

	pthread_mutex_lock( &reg_lock );
	while( reg->head == NULL )
	{
		pthread_cond_wait( &reg_cond, &reg_lock );
	}

	while( tmp != NULL )
	{
		if( tmp->ptr == elem )
		{
			pthread_cond_signal( &reg_cond );
			pthread_mutex_unlock( &reg_lock );
			return OP_NICK_ALREADY;
		}
		tmp = tmp->next;
	}
	pthread_cond_signal( &reg_cond );
	pthread_mutex_unlock( &reg_lock );

	if( tmp == NULL )
		return OP_NICK_UNKNOWN;

	return GENERIC_ERROR;
}//end is_registrated



/**
 * @brief	checks if the user 'user' is in the queue 'reg', if he is present return an error code, else registers hi
 * @param	user		is a pointer to user to register
 * @param 	reg		is a queue where are preset all users
 */
void sign_up( void *user, queue_t *reg )
{
	extern struct statistics chattyStats;

	if( is_registrated( reg, user ) == OP_NICK_UNKNOWN )//unregistrated user
	{
		pthread_mutex_lock( &reg_lock );
		push( reg, user );
		chattyStats.nusers += (int)1;
		pthread_mutex_unlock( &reg_lock );
	}
}

