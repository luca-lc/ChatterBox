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
/**
 * @struct			defines new data structure for hash table element
 * @var	nickname	var to save user nickname
 * @var	size		var to save the key used to calculate the hash value
 * @var	collision	pointer to queue of collision element
 */
typedef struct hash_elem
{
	char *nickname;
	int key;
}ht_elem_t;



/**
 * @struct			defines new data structure for hash table
 * @var	elem		pointer to hash table element
 * @var	size		var to save the length of hash table
 * @var n_elem		var to know the number of elements in hash table
 */
typedef struct ht
{
	ht_elem_t *elem;
	int size;
	int n_elem;
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
bool search( hashtable_t *table, char *name );



/**
 * @brief		removes a nickname from hash table
 * @var table	pointer to hash table where removing user
 * @var name	pointer to string where is saved nickname of user
 * @return		true if removes user
 * 				false otherwise
 */
bool removing( hashtable_t *table, char *name );



#endif /* HASHTABLE_H */
