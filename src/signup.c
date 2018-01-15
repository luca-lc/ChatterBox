/* $pool.c$ */

/**
 * @file signup.c
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c)2017 Luca Canessa (516639)                                    *
 *                                                                            *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * @section DESCRIPTION
 * Looking overview
 *
 * In this file there are functions to manipulate user registering
 *
 * checkin()	:	Inserts a nickname in hash table
 * 					Requires a pointer to data structure where saved pointer to
 * 					hash table and pointer to string where is the nickname to
 * 					insert
 * 					Returns true if terminates putting the user in the hash table
 * 					false otherwise
 *
 * delete()		:	Deletes a nickname from hash table
 * 					Requires a pointer to data structure where saved pointer to
 * 					hash table and pointer to string where is nickname to
 * 					remove
 * 					Returns a bool value where 'true' represented by 1 and
 * 					'false' by 0
*/


/******************************************************************************
									HEADER
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <src/queue.h>
#include <src/ops.h>
#include <src/stats.h>
#include <src/signup.h>


pthread_mutex_t reg_lock = PTHREAD_MUTEX_INITIALIZER;



/******************************************************************************
									FUNCTIONS
******************************************************************************/
/**
 * @brief		enters the user in the hash table to record it
 * @var arg		pointer to data structure where saved the pointer to hash table
 * 				and pointer to string where saved the nickname to register
 * @return 		true if user is logged
 * 				false otherwise
 */
bool checkin( checkin_arg *arg )
{
	bool out = false;

	if( search( arg->myt, arg->name ) == NULL )
	{
		pthread_mutex_lock( &reg_lock );
			out = insert( arg->myt, arg->name );
		pthread_mutex_unlock( &reg_lock );
		return out;
	}
	else
	{
		fprintf( stderr, "%s already exists\n", arg->name );
		return out;
	}
	return out;
}



/**
 * @brief		deletes the user from hash table
 * @var arg		pointer to data structure where saved the pointer to hash table
 * 				and pointer to string where saved the nickname to remove
 * @return		true if user is removed
 * 				false otherwise
 */
bool delete( checkin_arg *arg )
{
	bool out = false;
	if( search( arg->myt, arg->name) != NULL )
	{
		out = removing( arg->myt, arg->name );
	}

	return out;
}
