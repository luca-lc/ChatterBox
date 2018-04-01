/** $hashtable_h$ **/

/**
 * \file hashtable.h
 * \section LICENSE
 * ****************************************************************************
 * \author Luca Canessa (516639)                                    		  *
 *                                                                            *
 * \copyright \n															  *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * \section DESCRIPTION
 * Looking overview
 * 
 * This file contains prototypes and data structures to manage and manipulate a
 * hash table in which ChatterBox's users and groups will be saved.
 * 
 * initTable()	:	Creates an hash table and initializes its items.
 * 					Requires the length of the table.
 * 					Returns th pointer to new hash table.
 * 
 * insert()		:	Adds an user into the hash table using his nickname.
 * 					Requires the hash table pointer and the pointer to the 
 * 					user's name.
 * 					Returns true if inserted, otherwise false.
 * 
 * search()		:	Search an user into the hash table and if found then return
 * 					its pointer, else return NULL.
 * 					Requires the hash table pointer and the pointer to the 
 * 					user's name.
 * 					Returns the pointer to user if found, else NULL.
 * 
 * removing()	:	Search an user into the hash table and if found deletes it 
 * 					from table.
 * 					Requires pointer to the hash table and pointer to user's 
 * 					nickname.
 * 					Returns 1 if it exits without errors, else 0.
 * 
 * addGroup()	:	Allocates space to insert in hash table a chatting group.
 * 					WARNING: USING WITH LOCK ACQUIRED
 * 					Requires pointer to hash table and pointer to group's 
 * 					nickname.
 * 					Returns 1 if the group is added, otherwise 0.
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
 * 
 */


/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#ifndef HASHTABLE_H
#define HASHTABLE_H



#include <src/queue.h>
#include <src/pool.h>
#include <src/connections.h>
#include "src/chatty.h"



/******************************************************************************
							   STRUCTURES & TYPES
******************************************************************************/
extern int _MAX_HIST; ///< _MAX_HIST is number of history length



/**
 * \typedef				user_t
 * \struct				struct us
 * \brief 				Describes an user with the data structures
 * \var		nickname	Variable used to save the name of user
 * \var		fd_online	Variable used to know if the user is online or not
 * \var		alrdt_read	Variable used to know if the user has already read the 
 * 						messages history
 * \var		chats		Pointer to the queue where saved the messages history
 * \var		mygroup		Pointer to the queue to groups where user is logged
 * \var		us_lock		Variable used to lock the structure
 */
typedef struct us
{
	char nickname[MAX_NAME_LENGTH];
	int fd_online;
	unsigned int alrdy_read;
	queue_t *chats;
	queue_t *mygroup;
	pthread_mutex_t us_lock;
}user_t;



/**
 * \typedef				ht_elem_t
 * \struct				struct elem
 * \brief				Describes the user hash table element
 * \var		user		Pointer to user data structure
 * \var		collision	Pointer to queue where save the users with same hash 
 * 						value
 */
typedef struct elem
{
	user_t *user;
	queue_t *collision;
}ht_elem_t;



/**
 * \typedef				ht_G_t
 * \struct				struct gr
 * \brief				Describes the group hash table element
 * \var		group		Pointer to chat group
 * \var		collision	Pointer to queue where save the groups with same hash
 * 						value
 */
typedef struct gr
{
	group_chat_t *group;
	queue_t *collision;
}ht_G_t;



/**
 * \typedef				hashtable_t
 * \struct				struct ht
 * \brief				Describes the entire system where save users and groups
 * \var		ht_lock		Variable used to lock the entire hash table
 * \var		users		Pointer to users data structure
 * \var		groups		Pointer to groups data structure
 * \var		active_user	Pointer to the queue where saved all online users
 * \var		reg_users	Variable used to know how many users are logged
 */
typedef struct ht
{
	pthread_mutex_t ht_lock;
	ht_elem_t *users;
	ht_G_t *groups;
	queue_t *active_user;
	int max_u;
	int reg_users;
}hashtable_t;



/******************************************************************************
								   FUNCTIONS
******************************************************************************/
/**
 * \fn				initTable
 * \brief			Creates and initializes the hash table and set all parameters
 * \var		length	Number of elements to insert in hash table
 * \return			Pointer to hash table created
 */
hashtable_t *initTable( unsigned int length );



/**
 * \fn				insert
 * \brief			Inserts an user into the hash table
 * \warning			IT MUST USED WITH LOCK ACQUIRED
 * \var		table	Pointer to hash table where insert the new element
 * \var		name	Pointer to user nickname
 * \return			1 if terminate without errors otherwise 0
 */
int insert( hashtable_t *table, char *name );



/**
 * \fn				search
 * \brief			Search an user in hash table
 * \warning			IT MUST USED WITH LOCK ACQUIRED
 * \var		table	Pointer to hash table where search user
 * \var 	name	Pointer to user nickname
 * \return			Pointer to user found or NULL if not found
 */
user_t *search( hashtable_t *table, char *name );



/**
 * \fn				removing
 * \brief			Search an user in hash table and removes it
 * \warning			IT MUST USED WITH LOCK ACQUIRED
 * \var		table	Pointer to hash table where search user
 * \var 	name	Pointer to user nickname
 * \return			1 if the user was removed, otherwise 0
 */
int removing( hashtable_t *table, char *name );



/**
 * \fn				addGroup
 * \brief			Inserts a group into the hash table
 * \warning			IT MUST USED WITH LOCK ACQUIRED
 * \var		table	Pointer to hash table where insert the new element
 * \var		name	Pointer to group name
 * \return			1 if terminate without errors otherwise 0
 */
int addGroup( hashtable_t *table, char *name );



/**
 * \fn				searchGroup
 * \brief			Search a group in hash table
 * \warning			IT MUST USED WITH LOCK ACQUIRED
 * \var		table	Pointer to hash table where search group
 * \var 	name	Pointer to group nickname
 * \return			Pointer to group found or NULL if not found
 */
group_chat_t *searchGroup( hashtable_t *table, char *name );



/**
 * \fn				removingGroup
 * \brief			Search a group in hash table and removes it
 * \warning			IT MUST USED WITH LOCK ACQUIRED
 * \var		table	Pointer to hash table where search user
 * \var 	name	Pointer to group nickname
 * \return			1 if the group was removed, otherwise 0
 */
int removingGroup( hashtable_t *table, char *name );



#endif /* HASHTABLE_H */
