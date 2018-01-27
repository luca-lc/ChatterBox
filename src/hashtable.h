/* $hashtable.h$ */
/**
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c)2017 Luca Canessa (516639)                                    *
 *                                                                            *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
*/



#ifndef HASHTABLE_H
#define HASHTABLE_H



/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#include <src/queue.h>
#include <src/pool.h>
#include "src/chatty.h"



/******************************************************************************
							   STRUCTURES & TYPES
******************************************************************************/
//
extern int _MAX_HIST;



/**
 *
 */
typedef struct us
{
	char nickname[MAX_NAME_LENGTH];
	int key;
	queue_t *chats;
}user_t;



/**
 *
 */
typedef struct elem
{
	user_t *user;
	queue_t *collision;
}ht_elem_t;



/**
 *
 */
typedef struct ht
{
	pthread_mutex_t ht_lock;
	ht_elem_t *users;
	queue_t *active_user;
	queue_t *groups;
	int size;
	int reg_users;
}hashtable_t;



/******************************************************************************
								   FUNCTIONS
******************************************************************************/
/**
 * @brief		creates and initializes the hash table and set all parameters
 * @var	length	number of cells to insert in hash table
 * return		pointer to hash table created
 */
hashtable_t *initTable( unsigned int length );



/**
 * @brief		inserts an element in hash table (hash value is calculated from first letter of string)
 * @var	table	pointer to hash table where insert the new element
 * @var	name	pointer to string that represents the nickname to insert
 * @return		true if terminate without errors
 * 				false otherwise
 */
bool insert( hashtable_t *table, char *name );



/**
 * @brief		searches a nickname in hash table
 * @var	table	pointer to hash table where search user
 * @var name	pointer to string where is written the nickname to search
 * @return		true if user is present
 * 				false otherwise
 */
user_t *search( hashtable_t *table, char *name );



/**
 *
 */
bool removing( hashtable_t *table, char *name );



#endif /* HASHTABLE_H */
