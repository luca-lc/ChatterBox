/* $hashtable.c$ */

/**
 * @file hashtable.c
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
 * In this file there are functions to create and manipulate hash tables to
 * register users
 *
 * hashValue() :	Calculates the hash value using division
 * 					Requires value of key represented from the first letter of
 * 					nickname of type integer
 * 					Returns the hash value calculated using key and table's
 * 					length
 *
 * initTable() :	Creates and initializes the hash table and set its
 * 					parameters: size as length of table and n_elem to 0
 * 					Requires the length of hash table with type UNISGNED
 * 					INTEGER
 * 					Return pointer to hash table
 *
 * insert()    :	Inserts element user in hash table
 * 					Requires pointer to hash table where insert user and pointer
 * 					to string where is saved the name of user
 * 					Returns a bool type declared in 'chatty.h' file where
 * 					'true' is represented by 1 and 'false' by 0
 *
 * search()		:	Search a nickname in hash table using
 * 					Requires pointer to hash table where search and pointer to
 * 					string that represents the nickname to search
 * 					Returns a bool type declared in 'chatty.h' file where
 * 					'true' is represented by 1 and 'false' by 0
*/


/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <src/pool.h>
#include "src/hashtable.h"
#include <string.h>



/******************************************************************************
								   FUNCTIONS
******************************************************************************/

pthread_mutex_t hash_lock = PTHREAD_MUTEX_INITIALIZER;	///< declaration of mutex to lock hash table



/**
 * @brief		given the key calculates hash value with division method using the key and the max connection
 * @var	key		value of key => first letter of nickname
 * @return		hash value calculated with division method from key value and number of cells of hash table
 */
int hashVal( int key )
{
	return ( key % _MAX_CONN );
}



/**
 * @brief		creates and initializes the hash table and set all parameters as the size of table and number of elements in the table
 * @var	length	number of cells to insert in hash table
 * return table	pointer to hash table created
 */
hashtable_t *initTable( unsigned int length )
{
	hashtable_t *table;
	if( (table = ( hashtable_t * )malloc( sizeof( hashtable_t ) )) == NULL )
	{
		fprintf( stderr, "Problem to create hash table" );
		return NULL;
	}

	if( (table->elem = ( ht_elem_t * )malloc( length * sizeof( ht_elem_t ) )) == NULL )
	{
		fprintf( stderr, "Problem to create elements in hash table" );
		free( table );
		return NULL;
	}

	for( int i = 0; i < length; i++ )
	{
		table->elem[i].nickname = NULL;
	}


	table->size = length;
	table->n_elem = 0;

	return table;
}



/**
 * @brief		inserts an element in hash table using hash value calculated in 'hashValue' function
 * @var	table	pointer to hash table where insert the new element
 * @var	name	pointer to string that represents the nickname to insert
 * @return		true if terminate without errors
 * 				false otherwise
 */
bool insert( hashtable_t *table, char *name )
{
	int val = hashVal( name[0] );

	if( table->elem[val].nickname == NULL )
	{
		pthread_mutex_lock( &hash_lock );
		table->elem[val].nickname = (char *)malloc( 250 * sizeof( char ) );
		strcpy( table->elem[val].nickname, name );
		table->elem[val].key = name[0];
		table->elem[val].collision = NULL;
		table->n_elem += 1;
		pthread_mutex_unlock( &hash_lock );
		return true;
	}
	else
	{
		if( table->elem[val].collision == NULL )
		{
			table->elem[val].collision = initialQueue();
		}
		ht_elem_t *tmp = (ht_elem_t *)malloc( sizeof( ht_elem_t ) );
		tmp->nickname = ( char * )malloc( 250 * sizeof( char ) );
		strcpy(tmp->nickname, name);
		tmp->key = name[0];
		tmp->collision = NULL;
		if( push( table->elem[val].collision, tmp ) == 0 )
		{
			table->n_elem += 1;
			return true;
		}
		else
		{
			fprintf( stderr, "Problem to registering the user\n" );
			free( tmp );
			return false;
		}
	}

	printf( "puppa" );
	return false;
}



/**
 * @brief		searches an element that represented from user in the hash table
 * @var	table	pointer to hash table where search user
 * @var name	pointer to string where is written the name of user to search
 * @return		true if user is present
 * 				false otherwise
 */
bool search( hashtable_t *table, char *name )
{
	int val = hashVal( name[0] );

	if( table->elem[val].nickname != NULL )
	{
		if( strcmp(table->elem[val].nickname, name) == 0 )
		{
			return true;
		}
		else
		{
			ht_elem_t *tmp = NULL;
			if( table->elem[val].collision != NULL && table->elem[val].collision->head != NULL )
			{
				node_t *nt = table->elem[val].collision->head;
				tmp = nt->ptr;
				while( strcmp(tmp->nickname,name) != 0 && nt != NULL )
				{
					tmp = nt->ptr;
					nt = nt->next;
				}
				if( strcmp(tmp->nickname, name) == 0 )
				{
					return true;
				}
			}
		}
	}

	return false;
}



/**
 *
 */
bool removing( hashtable_t *table, char *name )
{
	int val = hashVal( name[0] );

	//removes nickname that is in hash table
	if( strcmp( table->elem[val].nickname, name) == 0 )
	{
		if( table->elem[val].collision != NULL ) //replaces this nickname with the first element in the overflow queue
		{
			ht_elem_t *first_elem = pull( table->elem[val].collision );

			pthread_mutex_lock( &hash_lock );
			strcpy( table->elem[val].nickname, first_elem->nickname );
			pthread_mutex_unlock( &hash_lock );

			free( first_elem );

			return true;
		}
		else //replaces nickname with NULL
		{
			table->elem[val].nickname = NULL;
			return true;
		}
	}
	else //removes nickname that is in the overflow queue
	{
		ht_elem_t *e = NULL;
		if( table->elem[val].collision != NULL && table->elem[val].collision->head != NULL ) //if there is at least one element in the overflow queue
		{
			node_t *node = table->elem[val].collision->head;
			e = node->ptr;
			node_t *prev = NULL;

			while( strcmp( e->nickname, name) != 0 && node != NULL ) //selects the node of the queue
			{
				prev = node;
				node = node->next;
				e = node->ptr;
			}

			if( node != NULL )
			{
				if( prev == NULL ) //if the first element
				{
					pthread_mutex_lock( &hash_lock );
					table->elem[val].collision->head = table->elem[val].collision->head->next;
					free(node);
					pthread_mutex_unlock( &hash_lock );

					return true;
				}
				else //if is in the queue in some position
				{
					pthread_mutex_lock( &hash_lock );
					prev->next = node->next;
					free( node );
					pthread_mutex_unlock( &hash_lock );

					return true;
				}
			}
		}
	}

	return false;
}
