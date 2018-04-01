/** $hashtable_c$ **/

/**
 * \file hashtable.c
 * \section LICENSE
 * ****************************************************************************
 * \author Luca Canessa (516639)                   			                  *
 *                                                                            *
 * \copyright \n															  *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * \section DESCRIPTION
 * Looking overview
 *
 * In this file there are functions to create and manipulate hash tables to
 * register users.
 * This hash table is  designed as an array in which each cell represents an 
 * element in which a user and a group or more are saved. So one or more users 
 * or groups could be saved in an array cell. This is managed with a linked list. 
 * If a user U or a group G is the first to be added to cell C, it is saved 
 * directly to the cell, otherwise U or G is appended to the  overflow queue C.
 * ALL FUNCTIONS (except hashValue() and initTable()) MUST BE USED WITH HASH 
 * TABLE's LOCK ACQUIRED
 *
 * hashValue() :	Calculates the hash value using division method with table
 * 					length
 * 					Requires value of key: first letter of nickname of type 
 * 					integer
 * 					Returns the hash value calculated
 *
 * initTable() :	Creates and initializes the hash table and its items.
 * 					Requires the length of hash table of type UNISGNED
 * 					INTEGER
 * 					Return pointer to hash table created.
 *
 * insert()    :	Inserts an user element in hash table.
 * 					Requires pointer to hash table where insert user and 
 * 					pointer to user nickname
 * 					Returns 1 if it has been added, otherwise 0.
 *
 * search()		:	Search a user in hash table using its nickname
 * 					Requires pointer to hash table where search and pointer to
 * 					user nickname
 * 					Returns pointer to user if found, else NULL.
 * 
 * removing()	:	Search an user in hash table and if found it, removes it.
 * 					Requires pointer to the table and pointer to users nickname
 * 					Returns 1 if it has been removed, otherwise 0
 * 
 * addGroup()	:	Allocates space to insert a group and initializes its items
 * 					Requires pointer to the hash table and pointer to group name
 * 					Returns 1 if the gruoup has been added, otherwise 0
 * 
 * searchGroup():	Search in the hash table a group using nickname.
 * 					Requires pointer to the hash table and pointer to group's 
 * 					nickname.
 * 					Returns pointer to the group found, otherwise NULL.
 * 
 * removingGroup():	Search the group with nickname passed and if found removes
 * 					it.
 * 					Requires pointer to the hash table and pointer to the group
 * 					nickname.
 * 					Returns 1 if it removed, otherwise 0.
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

int _MAX_HIST;		///< Variable used to save the size of users chat history



/**
 * \fn				hashVal
 * \brief			Given the key calculates hash value with division method using
 * 					the table size
 * \param	key		Value of key represented from the first character of name
 * \return			The hash value calculated
 */
int hashVal( int key )
{
	return ( key % _MAX_CONN );
}



/**
 * \fn				initTable
 * \brief			Creates and initializes the hash table and set all 
 * 					parameters. Allocates space for users and groups.
 * 					Allocates space also to save the online users
 * \param	length	Size of hash table. It's used for users and group
 * \return	table	Pointer to hash table just created
 */
hashtable_t *initTable( unsigned int length )
{
	hashtable_t *table;

	if( (table = ( hashtable_t * )malloc( sizeof( struct ht ) )) == NULL )
	{
		fprintf( stderr, "Problem to create hash table" );
		return NULL;
	}

	if( (table->users = ( ht_elem_t * )malloc( length * sizeof( struct elem ) )) == NULL )
	{
		fprintf( stderr, "Problem to create elements in hash table" );
		free( table );
		return NULL;
	}

	if( (table->groups = ( ht_G_t * )calloc( length, sizeof( struct gr ) )) == NULL )
	{
		fprintf( stderr, "Problem to create elements in hash table" );
		free( table->users );
		free( table );
		return NULL;
	}

	for( int i = 0; i < length; i++ )
	{
		table->users[i].user = NULL;
		table->users[i].collision = NULL;
		table->groups[i].group = NULL;
		table->groups[i].collision = NULL;
	}

	table->ht_lock = ( pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
	table->max_u = length;
	table->reg_users = 0;
	table->active_user = initialQueue();

	return table;
}



/**
 * \fn				insert
 * \brief			Inserts an element in hash table using hash value 
 * 					calculated in 'hashValue' function. Creates an users and 
 * 					fills all its items. Initializes also its queue pointers:
 * 					to chat hisoty and groups archive. If this user is the 
 * 					first in that cell then puts it directly in the table, else
 * 					inserts it in the overflow queue. 
 * \warning			!!! IMPORTANT: acquire mutex before using this !!!
 * \param	table	Pointer to hash table where insert the new element
 * \param	name	Pointer to user name
 * \return	0/1		If user has been inserted then 1, else 0		
 */
int insert( hashtable_t *table, char *name )
{

	int val = hashVal( name[0] );

	if( table->users[val].user == NULL )
	{
		if( (table->users[val].user = ( user_t * )calloc(1, sizeof( struct us ) )) == NULL )
		{
			return 0;
		}

		strcpy( table->users[val].user->nickname, name );

		table->users[val].user->chats = initialQueue();

		table->users[val].user->fd_online = -1;
		table->users[val].user->alrdy_read = 0;

		table->users[val].user->mygroup = initialQueue();

		table->reg_users += 1;

		return 1;
	}
	else
	{
		if( table->users[val].collision == NULL )
		{
			table->users[val].collision = initialQueue();
		}

		user_t *tmp;
		if( (tmp = ( user_t * )calloc(1, sizeof( user_t ) )) == NULL )
		{
			return 0;
		}

		strcpy(tmp->nickname, name);

		tmp->fd_online = -1;
		tmp->alrdy_read = 0;

		tmp->chats = initialQueue();
		tmp->mygroup = initialQueue();

		if( push( table->users[val].collision, tmp ) == 0 )
		{
			table->reg_users += 1;
			return 1;
		}
		else
		{
			destroy_queue( tmp->chats );
			free( tmp );
			return 0;
		}
	}
	return 0;
}



/**
 * \fn				search
 * \brief			Search an user using its name passed as argument in hash table:
 * 					if found it then return its pointer. To search, it starts from
 * 					the cell with hash value calculated and then goes into the
 * 					overflow queue.
 * \warning			!!! IMPORTANT: acquire mutex before using !!!
 * \param	table	Pointer to hash table where searching user
 * \param 	name	Pointer to user name to search
 * \return			Pointer to user found if exists, otherwise NULL
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
				node_t *nt = table->users[val].collision->head;
				user_t *tmp = NULL;

				while( nt != NULL )
				{
					tmp = nt->ptr;

					if( tmp != NULL && strcmp( tmp->nickname, name ) == 0 )
					{
						break;
					}
					else
					{
						nt = nt->next;
					}
				}

				if( nt == NULL )
				{
					return NULL;
				}
				else
				{
					return tmp;
				}
			}
		}
	}
	return NULL;
}



/**
 * \fn				removing
 * \brief			Search an user in hash table and if it found, it's removed.
 * 					The operations to search user is the same of the function
 * 					'search()'. Instead, to remove the user, the function check 
 * 					if the user that must be removed is located directly in the 
 * 					table then check if exists an user into overflow queue, if 
 * 					it exists then swap the users, else  remove the user. If 
 * 					the user to be removed is in the overflow queue then 
 * 					removes the node of queue.
 * \warning			!!! IMPORTANT: acquire mutex before using !!!
 * \param	table	Pointer to hash table
 * \param	name	Pointer to user name
 * \Return	1/0		1 if the user has been removed, else 0
 */
int removing( hashtable_t *table, char *name )
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

		while( tmp->mygroup != NULL )
			{
				group_chat_t *g = pull( tmp->mygroup );
				if( g != NULL )
					{
						node_t *u = g->participants->head;

						while( u != NULL )
							{
								user_t *us = u->ptr;
								if( strcmp( us->nickname, name ) == 0 )
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
				else
					{
						break;
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

		return 1;
	}
	else
	{
		if( table->users[val].collision != NULL )
		{
			node_t *n = table->users[val].collision->head;
			user_t *u = NULL;

			while( n != NULL )
				{
					u = n->ptr;
					if( u != NULL && strcmp( u->nickname, name) == 0 )
						{
							remove_node( table->users[val].collision, n );
							break;
						}
					else
						{
							n = n->next;
						}
				}


			if( table->users[val].collision->head->ptr == NULL )
				{
					free( table->users[val].collision->head );
					free( table->users[val].collision );
					table->users[val].collision = NULL;
				}

			if( n == NULL )
				{
					return 0;
				}
			else
				{
					table->reg_users -= 1;
					return 1;
				}
		}
	}

	return 0;
}



/**
 * \fn				addGroup
 * \brief			Inserts an element in hash table using hash value 
 * 					calculated in 'hashValue' function. Creates a group and 
 * 					fills all its items. Initializes also its queue pointers:
 * 					to chat hisoty and partecipants queue. If this group is the 
 * 					first in that cell then puts it directly in the table, else
 * 					inserts it in the overflow queue. 
 * \warning			!!! IMPORTANT: acquire mutex before using this !!!
 * \param	table	Pointer to hash table where insert the new element
 * \param	name	Pointer to group name
 * \return	0/1		If user has been inserted then 1, else 0		
 */
int addGroup( hashtable_t *table, char *name )
{

	int val = hashVal( name[0] );

	if( table->groups[val].group == NULL )
	{
		if( (table->groups[val].group = (group_chat_t *)malloc( sizeof( struct grp_msg ) )) == NULL )
		{
			return 0;
		}

		strcpy( table->groups[val].group->chat_title, name );

		table->groups[val].group->participants = initialQueue();
		table->groups[val].group->messages = initialQueue();

		return 1;
	}
	else
	{
		if( table->groups[val].collision == NULL )
		{
			table->groups[val].collision = initialQueue();
		}

		group_chat_t *tmp;
		if( (tmp = (group_chat_t *)malloc( sizeof( struct grp_msg ) )) == NULL )
		{
			return 0;
		}

		strcpy(tmp->chat_title, name);

		tmp->messages = initialQueue();
		tmp->participants = initialQueue();


		if( push( table->groups[val].collision, tmp ) == 0 )
		{
			return 1;
		}
		else
		{
			destroy_queue( tmp->messages );

			free( tmp );
			return 0;
		}
	}
	return 0;
}



/**
 * \fn				searchGroup
 * \brief			Search a group using its name passed as argument in hash table:
 * 					if found it then return its pointer. To search, it starts from
 * 					the cell with hash value calculated and then goes into the
 * 					overflow queue.
 * \warning			!!! IMPORTANT: acquire mutex before using !!!
 * \param	table	Pointer to hash table where searching user
 * \param 	name	Pointer to group name to search
 * \return			Pointer to group found if exists, otherwise NULL
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
			if( table->groups[val].collision != NULL )
			{
				group_chat_t *tmp = NULL;
				node_t *nt = table->groups[val].collision->head;
				while( nt != NULL )
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
				if( nt != NULL )
					{
						return tmp;
					}
			}
		}
	}
	return NULL;
}



/**
 * \fn				removingGroup
 * \brief			Search a group in hash table and if it found, it's removed.
 * 					The operations to search a group is the same of the 
 * 					function 'search()'. Instead, to remove the group, the 
 * 					function check if the group that must be removed is located 
 * 					directly in the table then check if exists a group into 
 * 					overflow queue, if it exists then swap the group, else 
 * 					remove the user. If the user to be removed is in the 
 * 					overflow queue then removes the node of queue.
 * \warning			!!! IMPORTANT: acquire mutex before using !!!
 * \param	table	Pointer to hash table
 * \param	name	Pointer to group name
 * \Return	1/0		1 if the group has been removed, else 0
 */
int removingGroup( hashtable_t *table, char *name )
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
				pull( tmp->participants );
			}
		destroy_queue( tmp->participants );


		table->groups[val].group = NULL;
		free( tmp );

		if( subst != NULL )
		{
			table->groups[val].group = subst;
		}
			
		return 1;
	}
	else
	{
		if( table->groups[val].collision != NULL )
		{

			node_t *n = table->groups[val].collision->head;
			group_chat_t *t_g = NULL;

			while( n != NULL )
				{
					t_g = n->ptr;
					if( t_g != NULL && strcmp( t_g->chat_title, name ) == 0 )
						{
							remove_node( table->groups[val].collision, n );
							break;
						}
					else
						{
							n = n->next;
						}
				}

			if( table->groups[val].collision->head->ptr == NULL )
				{
					free( table->groups[val].collision->head );
					free( table->groups[val].collision );
					table->groups[val].collision = NULL ;
				}

			if( n == NULL )
				{
					return 0;
				}
			else
				{
					return 1;
				}
		}
	}

	return 0;
}
