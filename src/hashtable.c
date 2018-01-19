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
#include <string.h>
#include <src/pool.h>
#include <src/hashtable.h>




/******************************************************************************
								   FUNCTIONS
******************************************************************************/

int _MAX_HIST;


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
	//creation table
	if( (table = ( hashtable_t * )malloc( sizeof( hashtable_t ) )) == NULL )
	{
		fprintf( stderr, "Problem to create hash table" );
		return NULL;
	}
	//creation cell
	if( (table->users = ( ht_elem_t * )malloc( length * sizeof( ht_elem_t ) )) == NULL )
	{
		fprintf( stderr, "Problem to create elements in hash table" );
		free( table );
		return NULL;
	}
	//init cell
	for( int i = 0; i < length; i++ )
	{
		table->users[i].user = NULL;
		table->users[i].collision = NULL;
	}
	//init elem table
	table->ht_lock = ( pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
	table->size = length;
	table->reg_users = 0;
	queue_t *active_user = initialQueue();

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

	if( table->users[val].user == NULL )
	{

		pthread_mutex_lock( &table->ht_lock );
			if( (table->users[val].user = (user_t *)malloc( sizeof( user_t ) )) == NULL )
			{

				pthread_mutex_unlock( &table->ht_lock );
				return false;
			}

			if( (table->users[val].user->nickname = (char *)malloc( MAX_NAME_LENGTH * sizeof( char ) )) == NULL )
			{
				pthread_mutex_unlock( &table->ht_lock );
				return false;
			}
			strcpy( table->users[val].user->nickname, name );

			table->users[val].user->chats = initialQueue();

			table->users[val].user->key = val;

			table->reg_users += 1;

		pthread_mutex_unlock( &table->ht_lock );
		return true;
	}
	else
	{
		pthread_mutex_lock( &table->ht_lock );
			if( table->users[val].collision == NULL )
			{
				table->users[val].collision = initialQueue();
			}
		pthread_mutex_unlock( &table->ht_lock );

		user_t *tmp;
		if( (tmp = (user_t *)malloc( sizeof( user_t ) )) == NULL )
		{
			return false;
		}
		if( (tmp->nickname = ( char * )malloc( MAX_NAME_LENGTH * sizeof( char ) )) == NULL )
		{
			return false;
		}
		strcpy(tmp->nickname, name);

		tmp->key = val;

		tmp->chats = initialQueue();

		if( push( table->users[val].collision, tmp ) == 0 )
		{
			pthread_mutex_lock( &table->ht_lock );
				table->reg_users += 1;
			pthread_mutex_unlock( &table->ht_lock );

			return true;
		}
		else
		{
			fprintf( stderr, "Problem to registering the user\n" );
			free( tmp );
			return false;
		}
	}

	return false;
}



/**
 * @brief		searches an element that represented from user in the hash table
 * @var	table	pointer to hash table where search user
 * @var name	pointer to string where is written the name of user to search
 * @return		true if user is present
 * 				false otherwise
 */
user_t *search( hashtable_t *table, char *name )
{
	int val = hashVal( name[0] );

	if( table->users[val].user != NULL )
	{
		if( strcmp(table->users[val].user->nickname, name) == 0 )
		{
			return table->users[val].user;
		}
		else
		{
			user_t *tmp = NULL;
			if( table->users[val].collision != NULL && table->users[val].collision->head != NULL )
			{
				node_t *nt = table->users[val].collision->head;
				tmp = nt->ptr;
				while( strcmp(tmp->nickname, name) != 0 && nt != NULL )
				{
					tmp = nt->ptr;
					nt = nt->next;
				}

				if( strcmp(tmp->nickname, name) == 0 )
				{
					return tmp;
				}
			}
		}
	}

	return NULL;
}



/**
 *
 */
bool removing( hashtable_t *table, char *name )
{
	int val = hashVal( name[0] );

	//removes nickname that is in hash table
	if( strcmp( table->users[val].user->nickname, name) == 0 )
	{
		user_t *subst = NULL;
		if( table->users[val].collision != NULL )
		{
			subst = pull( table->users[val].collision );
		}
		user_t *tmp = table->users[val].user;
		pthread_mutex_lock( &table->ht_lock );
			free( tmp->nickname );
			free( tmp->chats );
			free( tmp );
			tmp->key = -1;
			table->users[val].user = NULL;
			if( subst != NULL )
			{
				table->users[val].user = subst;
			}
			table->reg_users -= 1;
		pthread_mutex_unlock( &table->ht_lock );

		return true;
	}
	else
	{
		user_t *tmp = NULL;
		if( table->users[val].collision != NULL && table->users[val].collision->head != NULL )
		{
			node_t *ne = table->users[val].collision->head->next;
			node_t *e = table->users[val].collision->head;
			tmp = e->ptr;
			while( strcmp(tmp->nickname, name) != 0 && ne != NULL )
			{
				e = ne;
				tmp = e->ptr;
				ne = ne->next;
			}

			if( strcmp(tmp->nickname, name) == 0 )
			{
				if( ne == NULL )
				{
					if( e->prev == NULL )
					{
						table->users[val].collision->head = e->next;
						table->users[val].collision->tail = e->prev;
						free( tmp->chats );
						free( tmp->nickname );
						tmp->key = -1;
						free( e );
						return true;
					}
					else
					{
						table->users[val].collision->tail = e->prev;
						e->prev->next = NULL;
						free( tmp->chats );
						free( tmp->nickname );
						tmp->key = -1;
						free( e );
						return true;
					}
				}
				else
				{
					if( e->prev == NULL )
					{
						table->users[val].collision->head = e->next;
						e->next->prev = NULL;
						free( tmp->chats );
						free( tmp->nickname );
						tmp->key = -1;
						free( e );
						return true;
					}
					else
					{
						e->prev->next = e->next;
						e->next->prev = e->prev;
						free( tmp->chats );
						free( tmp->nickname );
						tmp->key = -1;
						free( e );
						return true;
					}
				}

			}
		}
	}

	return false;
}
