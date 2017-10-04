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



/******************************************************************************
							   STRUCTURES & TYPES
******************************************************************************/
/**
 *
 */
typedef struct hash_elem
{
	char *nickname;
	int key;
	queue_t *collision;
}ht_elem_t;



/**
 *
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
void insert( hashtable_t *table, char *name );



#endif /* HASHTABLE_H */
