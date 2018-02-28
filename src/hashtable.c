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
#include <src/connections.h>



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

	if( (table->groups = ( ht_G_t * )malloc( length * sizeof( ht_G_t ) )) == NULL )
	{
		fprintf( stderr, "Problem to create elements in hash table" );
		free( table->users );
		free( table );
		return NULL;
	}

	//init cell
	for( int i = 0; i < length; i++ )
	{
		table->users[i].user = NULL;
		table->users[i].collision = NULL;
		table->groups[i].group = NULL;
		table->groups[i].collision = NULL;

	}

	//init elem table
	table->ht_lock = ( pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
	table->max_u = length;
	table->reg_users = 0;
	table->active_user = initialQueue();

	return table;
}



/**
 * @brief		inserts an element in hash table using hash value calculated in 'hashValue' function
 * 				!!! IMPORTANT: acquire mutex before using !!!
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
		if( (table->users[val].user = (user_t *)malloc( sizeof( user_t ) )) == NULL )
		{
			return false;
		}

		strcpy( table->users[val].user->nickname, name );

		table->users[val].user->chats = initialQueue();

		table->users[val].user->fd_online = -1;

		table->users[val].user->mygroup = initialQueue();

		table->reg_users += 1;

		return true;
	}
	else
	{
			if( table->users[val].collision == NULL )
			{
				table->users[val].collision = initialQueue();
			}

		user_t *tmp;
		if( (tmp = (user_t *)malloc( sizeof( user_t ) )) == NULL )
		{
			return false;
		}

		strcpy(tmp->nickname, name);

		tmp->fd_online = -1;

		tmp->chats = initialQueue();
		tmp->mygroup = initialQueue();

		if( push( table->users[val].collision, tmp ) == 0 )
		{
			table->reg_users += 1;
			return true;
		}
		else
		{
			destroy_queue( tmp->chats );
			free( tmp );
			return false;
		}
	}
	return false;
}



/**
 * @brief		inserts an element in hash table using hash value calculated in 'hashValue' function
 * 				!!! IMPORTANT: acquire mutex before using !!!
 * @var	table	pointer to hash table where insert the new element
 * @var	name	pointer to string that represents the nickname to insert
 * @return		true if terminate without errors
 * 				false otherwise
 */
bool addGroup( hashtable_t *table, char *name )
{

	int val = hashVal( name[0] );

	if( table->groups[val].group == NULL )
	{
		if( (table->groups[val].group = (group_chat_t *)malloc( sizeof( group_chat_t ) )) == NULL )
		{
			pthread_mutex_unlock( &table->ht_lock );
			return false;
		}

		strcpy( table->groups[val].group->chat_title, name );

		table->groups[val].group->participants = initialQueue();
		table->groups[val].group->messages = initialQueue();

		return true;
	}
	else
	{
		if( table->groups[val].collision == NULL )
		{
			table->groups[val].collision = initialQueue();
		}

		group_chat_t *tmp;
		if( (tmp = (group_chat_t *)malloc( sizeof( group_chat_t ) )) == NULL )
		{
			return false;
		}

		strcpy(tmp->chat_title, name);

		tmp->messages = initialQueue();
		tmp->participants = initialQueue();


		if( push( table->groups[val].collision, tmp ) == 0 )
		{
			return true;
		}
		else
		{
			destroy_queue( tmp->messages );

			free( tmp );
			return false;
		}
	}
	return false;
}


/**
 * @brief		searches an element that represented from user in the hash table
 * 				!!! IMPORTANT: acquire mutex before using !!!
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
			if( table->users[val].collision != NULL  )
			{
				user_t *tmp = NULL;
				node_t *nt = table->users[val].collision->head;
				while( nt != NULL && nt->ptr != NULL )
				{
					tmp = nt->ptr;

					if( strcmp( tmp->nickname, name ) == 0 )
					{
						break;
					}
					else
					{
						nt = nt->next;
					}
				}
				return tmp;
			}
		}
	}
	return NULL;
}



/**
 * @brief		searches an element that represented from user in the hash table
 * 				!!! IMPORTANT: acquire mutex before using !!!
 * @var	table	pointer to hash table where search user
 * @var name	pointer to string where is written the name of user to search
 * @return		true if user is present
 * 				false otherwise
 */
group_chat_t *searchGroup( hashtable_t *table, char *name )
{
	int val = hashVal( name[0] );

	if( table->groups[val].group != NULL )
	{
		if( strcmp(table->groups[val].group->chat_title, name) == 0 )
		{
			return table->groups[val].group;
		}
		else
		{
			if( table->groups[val].collision != NULL && table->groups[val].collision->head != NULL )
			{
				group_chat_t *tmp = NULL;
				node_t *nt = table->groups[val].collision->head;
				while( nt != NULL && nt->ptr != NULL )
				{
					tmp = nt->ptr;
					if( strcmp( tmp->chat_title, name ) == 0 )
					{
						break;
					}
					else
					{
						nt = nt->next;
					}
				}
				return tmp;
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

			while( tmp->mygroup->head != tmp->mygroup->tail )
				{
					group_chat_t *g = pull( tmp->mygroup );
					node_t *u = g->participants->head;
					int stop = 0;
						while( u != NULL && !stop )
							{
								user_t *n = u->ptr;
								if( strcmp( n->nickname, tmp->nickname ) == 0 )
									{
										remove_node( g->participants, u );
										break;
									}
								else
									{
										u = u->next;
									}
							}
				}
			destroy_queue( tmp->mygroup );


			tmp->fd_online = -1;
			table->users[val].user = NULL;

			if( subst != NULL )
			{
				table->users[val].user = subst;
			}

			table->reg_users -= 1;

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
						table->users[val].collision->head->ptr = NULL;
						table->users[val].collision->tail = table->users[val].collision->head;

						while( tmp->mygroup->head != tmp->mygroup->tail )
							{
								group_chat_t *g = pull( tmp->mygroup );
								node_t *u = g->participants->head;
								int stop = 0;
									while( u != NULL && !stop )
										{
											user_t *n = u->ptr;
											if( strcmp( n->nickname, tmp->nickname ) == 0 )
												{
													remove_node( g->participants, u );
													stop = 1;
												}
											else
												{
													u = u->next;
												}
										}
							}


						destroy_queue( tmp->mygroup );
						tmp->fd_online = -1;

						free( tmp );
						free( table->users[val].collision->head );
						free( table->users[val].collision );
						table->reg_users -= 1;

						return true;
					}
					else
					{
						table->users[val].collision->tail = e->prev;
						e->prev->next = NULL;

						while( tmp->mygroup->head != NULL )
							{
								group_chat_t *g = pull( tmp->mygroup );
								node_t *u = g->participants->head;
								int stop = 0;
									while( u != NULL && !stop )
										{
											user_t *n = u->ptr;
											if( strcmp( n->nickname, tmp->nickname ) == 0 )
												{
													remove_node( g->participants, u );
													stop = 1;
												}
											else
												{
													u = u->next;
												}
										}
							}

						destroy_queue( tmp->mygroup );
						tmp->fd_online = -1;
						free( e );
						table->reg_users -= 1;

						return true;
					}
				}
				else
				{
					if( e->prev == NULL )
					{
						table->users[val].collision->head = e->next;
						e->next->prev = NULL;

						while( tmp->mygroup->head != tmp->mygroup->tail )
							{
								group_chat_t *g = pull( tmp->mygroup );
								node_t *u = g->participants->head;
								int stop = 0;
									while( u != NULL && !stop )
										{
											user_t *n = u->ptr;
											if( strcmp( n->nickname, tmp->nickname ) == 0 )
												{
													remove_node( g->participants, u );
													stop = 1;
												}
											else
												{
													u = u->next;
												}
										}
							}

						destroy_queue( tmp->mygroup );
						tmp->fd_online = -1;
						free( e );
						table->reg_users -= 1;

						return true;
					}
					else
					{
						e->prev->next = e->next;
						e->next->prev = e->prev;

						while( tmp->mygroup->head != NULL )
							{
								group_chat_t *g = pull( tmp->mygroup );
								node_t *u = g->participants->head;
								int stop = 0;
									while( u != NULL && !stop )
										{
											user_t *n = u->ptr;
											if( strcmp( n->nickname, tmp->nickname ) == 0 )
												{
													remove_node( g->participants, u );
													stop = 1;
												}
											else
												{
													u = u->next;
												}
										}
							}

						destroy_queue( tmp->mygroup );
						tmp->fd_online = -1;
						free( e );
						table->reg_users -= 1;
						return true;
					}
				}

			}
		}
	}
	pthread_mutex_unlock( &table->ht_lock );

	return false;
}



/**
 *
 */
bool removingGroup( hashtable_t *table, char *name )
{
	int val = hashVal( name[0] );

	//removes nickname that is in hash table
	if( strcmp( table->groups[val].group->chat_title, name) == 0 )
	{
		group_chat_t *subst = NULL;
		if( table->groups[val].collision != NULL )
		{
			subst = pull( table->groups[val].collision );
		}

		group_chat_t *tmp = table->groups[val].group;
		destroy_queue( tmp->messages ); 

		while( tmp->participants->head->ptr != NULL )
			{
				remove_node( tmp->participants, tmp->participants->head );
			}
		destroy_queue( tmp->participants );

		table->groups[val].group = NULL;
		free( tmp );

		if( subst != NULL )
		{
			table->groups[val].group = subst;
		}
			
		return true;
	}
	else
	{
		group_chat_t *tmp = NULL;
		if( table->groups[val].collision != NULL && table->groups[val].collision->head != NULL )
		{
			node_t *ne = table->groups[val].collision->head->next;
			node_t *e = table->groups[val].collision->head;
			tmp = e->ptr;
			while( strcmp(tmp->chat_title, name) != 0 && ne != NULL )
			{
				e = ne;
				tmp = e->ptr;
				ne = ne->next;
			}

			if( strcmp(tmp->chat_title, name) == 0 )
			{
				if( ne == NULL )
				{
					if( e->prev == NULL )
					{
						table->groups[val].collision->head = e->next;
						table->groups[val].collision->tail = e->prev;

						destroy_queue( tmp->messages );
						destroy_queue( tmp->participants );
						
						free( e );
						return true;
					}
					else
					{
						table->groups[val].collision->tail = e->prev;
						e->prev->next = NULL;

						destroy_queue( tmp->messages );
						destroy_queue( tmp->participants );

						free( e );
						return true;
					}
				}
				else
				{
					if( e->prev == NULL )
					{
						table->groups[val].collision->head = e->next;
						e->next->prev = NULL;

						destroy_queue( tmp->messages );
						destroy_queue( tmp->participants );

						free( e );
						return true;
					}
					else
					{
						e->prev->next = e->next;
						e->next->prev = e->prev;

						destroy_queue( tmp->messages );
						destroy_queue( tmp->participants );

						free( e );
						return true;
					}
				}

			}
		}
	}

	return false;
}
