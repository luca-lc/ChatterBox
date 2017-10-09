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
	queue_t *collision;
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
hashtable_t *initTable( unsigned int length );
int insert( hashtable_t *table, char *name );
bool search( hashtable_t *table, char *name );



#endif /* HASHTABLE_H */
