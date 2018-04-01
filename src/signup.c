/** $signup_c$ **/

/**
 * \file signup.c
 * \section LICENSE
 * ****************************************************************************
 * \author Luca Canessa (516639)                                    		  *
 *                                                                            *
 * \copyright																  *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * \section DESCRIPTION
 * Looking overview
 *
 * In this file there are functions to manipulate user registering
 *
 * checkin()	:	Inserts a nickname in hash table
 * 					Requires a pointer to data structure where all users have 
 * 					been saved, in this case is an hash table designed in 
 * 					'hashtable.h' file, and pointer to string where is the 
 * 					nickname to insert
 * 					Returns '1' if it has been inserted into the table, 
 * 					'0' if exit with error, or 'OP_NICK_ALREADY' if user is
 * 					already in the table
 * 
 * connecting()	:	Search an user in hash table in the hash table
 * 					Requires a pointer to data structure (hash table designed 
 * 					in 'hashtable.h') and pointer to string where is nickname
 * 					Returns a pointer to user of type 'user_t' if exists,
 * 					else 'NULL'
 * 
 * delete()		:	Deletes an user from hash table
 * 					Requires a pointer to hash table (designed in 'hashtable.h') 
 * 					where saved the user and the user's nickname of type char *
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
 * \fn				checkin
 * \brief			Inserts a user of type 'user_t' in the hash table using an additional function called 'insert' if this user is not already present, \n
 * 					this checked with external function 'search'
 * \warning			!!! WARNING : the functions 'search()' and 'insert()' inside of 'connecting()' MUST be protected with table mutex variables !!!
 * \param 	users	'hashtable_t' pointer to the hash table, where all users are saved
 * \param 	nick	Pointer of type 'char *' to string of user's nickname
 * \return 	out		Returns 1 if the user has been registered, 0 if it exits with some error, OP_NICK_ALREADY (26) if the user has already been registered previously
 */
int checkin( hashtable_t *users, char *nick )
{
	int out = 0;
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
		out = OP_NICK_ALREADY;
		return out;
		}
}



/**
 * \fn				connecting
 * \brief			Search an 'user_t' user with nickname 'nick' in the table using function 'search' declared in 'hashtable.h' file
 * \warning			!!! WARNING : the function 'search()' inside 'connecting()' MUST be protected with table mutex variables !!!
 * \param 	users	Pointer of type 'hashtable_t' where all users are saved
 * \param 	nick	'char *' pointer to user with this 'nick' nickname
 * \return	user	'user_t' pointer that represent the user if it exists,'NULL' otherwise
 */
user_t *connecting( hashtable_t *users, char *nick )
{
	pthread_mutex_lock( &users->ht_lock );
		user_t *user = search( users, nick );
	pthread_mutex_unlock( &users->ht_lock );
	return user;
}



/**
 * \fn				delete
 * \brief			Removes the 'user_t' user from hash table using external  function 'removing' declared in 'hashtable.h', where has been declared \n
 * 					also the function 'search' used to know if this user exists
 * \warning			!!! WARNING : the functions 'search()' and 'delete()' inside of 'connecting()' MUST be protected with table mutex variables !!!
 * \param 	users	Pointer of type 'hashtable_t' where all users are saved
 * \param 	nick	'char *' pointer to user with this 'nick' nickname
 * \return	out		1 if user has been deleted, 0 otherwise
 */
int delete( hashtable_t *users, char *nick )
{
	int out = 0;
	pthread_mutex_lock( &users->ht_lock );
		if( search( users, nick ) != NULL )
		{
			out = removing( users, nick );
		}
	pthread_mutex_unlock( &users->ht_lock );

	return out;
}
