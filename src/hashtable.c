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
*/


/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <src/pool.h>
#include <src/hashtable.h>
#include <string.h>



/******************************************************************************
								   FUNCTIONS
******************************************************************************/
/**
 *
 */
int hashVal( int key )
{
	return ( key % max_conn );
}



/**
 *
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
 *
 */
void insert( hashtable_t *table, char *name )
{
	int val = hashVal( name[0] );

	if( table->elem[val].nickname == NULL )
	{
		table->elem[val].nickname = name;
		table->elem[val].key = val;
		table->elem[val].collision = NULL;
	}
	else
	{
		if( table->elem[val].collision == NULL )
		{
			table->elem[val].collision = initialQueue();
		}
		ht_elem_t *tmp = (ht_elem_t*)malloc( sizeof( ht_elem_t) );
		tmp->nickname = name;
		tmp->key = val;
		tmp->collision = NULL;
		push( table->elem[val].collision, tmp );
	}

	table->n_elem += 1;
}



/**
 *
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
					while( strcmp(tmp->nickname,name)!=0 && nt != NULL )
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


