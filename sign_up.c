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
 * @brief		checks if a element (user) is in the queue 'reg'
 * @param reg	pointer to queue of registered users
 * @param elem	pointer to user to check if is in the queue
 * @return 		OP_NICK_ALREADY if registered
 * 				OP_NICK_UNKNOWN if not registered
 */
op_t is_registered( queue_t *reg, void *elem )
{
	int isreg = 0;
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
			isreg = 1;
		}
		tmp = tmp->next;
	}
	pthread_cond_signal( &reg_cond );
	pthread_mutex_unlock( &reg_lock );

	if( isreg == 1 )
	{
		return OP_NICK_ALREADY;
	}
	else
	{
		return OP_NICK_UNKNOWN;
	}

}//end is_registered



/**
 * @brief 		checks if the user 'user' is registered, if not record him
 * @param user	pointer to user to record
 * @param reg	pointer to queue of registered users
 */
void sign_up( void *user, queue_t *reg )
{
	extern struct statistics chattyStats;

	if( is_registered( reg, user ) == OP_NICK_UNKNOWN )//unregistrated user
	{
		pthread_mutex_lock( &reg_lock );
		push( reg, user );
		chattyStats.nusers += (int)1;
		pthread_mutex_unlock( &reg_lock );
	}
}//end sign_up

