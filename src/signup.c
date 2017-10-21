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
 *
 */
bool checkin( checkin_arg *arg )
{
	bool out = false;
	if( (out = search( arg->myt, arg->name )) == false )
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
 *
 */
bool delete( checkin_arg *arg )
{
	bool out = false;
	if( search( arg->myt, arg->name) == true )
	{
		out = removing( arg->myt, arg->name );
	}

	return out;
}
