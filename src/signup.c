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
int checkin( hashtable_t *users, char *nick )
{
	int out = false;
	pthread_mutex_lock( &users->ht_lock );
		if( search( users, nick ) == NULL )
		{
			out = insert( users, nick );
	pthread_mutex_unlock( &users->ht_lock );
		return out;
		}
		else
		{
	pthread_mutex_unlock( &users->ht_lock );
		return OP_NICK_ALREADY;
		}
}



/**
 * @brief		deletes the user from hash table
 * @var users	pointer to data structure where saved the user list
 * @var nick	pointer to string where saved the nickname to remove
 * @return		true if user is removed
 * 				false otherwise
 */
user_t *connecting( hashtable_t *users, char *nick )
{
	pthread_mutex_lock( &users->ht_lock );
		user_t *user = search( users, nick );
	pthread_mutex_unlock( &users->ht_lock );
	return user;
}



/**
 * @brief		deletes the user from hash table
 * @var users	pointer to data structure where saved the user list
 * @var nick	pointer to string where saved the nickname to remove
 * @return		true if user is removed
 * 				false otherwise
 */
bool delete( hashtable_t *users, char *nick )
{
	bool out = false;
	pthread_mutex_lock( &users->ht_lock );
		if( search( users, nick ) != NULL )
		{
			out = removing( users, nick );
		}
	pthread_mutex_unlock( &users->ht_lock );

	return out;
}
