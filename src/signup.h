/** $signup_h$ **/

/**
 * \file signup.h
 * \section LICENSE
 * ****************************************************************************
 * \author Luca Canessa (516639)		                                      *
 * 																			  *
 * \copyright                                                                 *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * \section DESCRIPTION
 * Looking overview
 * 
 * In this file are presents the prototypes of functions to manage users 
 * registering
 * 
 * checkin()	:	Inserts a nickname in hash table
 * 					Requires a pointer to data structure where all users have 
 * 					been saved, designed in 'hashtable.h' file, and pointer to 
 * 					string where is the nickname to insert
 * 					Returns '1' if it has been inserted into the table, 
 * 					'0' if exit with error, or 'OP_NICK_ALREADY' if user is
 * 					already in the table
 * 
 * connecting()	:	Search an user in hash table
 * 					Requires a pointer to data structure (hash table designed 
 * 					in 'hashtable.h') and pointer to string where is nickname
 * 					Returns a pointer to user of type 'user_t' if exists,
 * 					else 'NULL'
 * 
 * delete()		:	Deletes an user from hash table
 * 					Requires a pointer to hash table (designed in 'hashtable.h') 
 * 					where saved the user and the user's nickname of type char *
 * 					Returns an int: 1 if the user has been deleted, else 0.
*/


/******************************************************************************
									HEADER
******************************************************************************/
#ifndef SIGNUP_H_
#define SIGNUP_H_

#include <src/ops.h>
#include <src/hashtable.h>
#include "src/chatty.h"


/******************************************************************************
									FUNCTIONS
******************************************************************************/
/**
 * \fn				checkin
 * \brief			Inserts a user with the nickname 'nick' in the hash table 'hashtable_t'
 * \param 	users	'hashtable_t' pointer to the hash table, where all users are saved
 * \param 	nick	Pointer of type 'char *' to string of user's nickname
 * \return 			Returns 1 if the user has been registered, 0 if it exits with some error, OP_NICK_ALREADY (26) if the user has already been registered previously
 */
int checkin( hashtable_t *users, char *nick );



/**
 * \fn			connecting
 * \brief		Search an 'user_t' user with nickname 'nick' in the table
 * \param users	Pointer of type 'hashtable_t' where all users are saved
 * \param nick	'char *' pointer to user with this 'nick' nickname
 * \return		'user_t' pointer that represent the user if it exists,'NULL' otherwise
 */
user_t *connecting( hashtable_t *users, char *nick );



/**
 * \fn			delete
 * \brief		Removes if it exists the 'user_t' user from hash table
 * \param users	Pointer of type 'hashtable_t' where all users are saved
 * \param nick	'char *' pointer to user with this 'nick' nickname
 * \return		1 if user has been deleted, 0 otherwise
 */
int delete( hashtable_t *users, char *nick );



#endif /* SIGN_UP_H_ */
