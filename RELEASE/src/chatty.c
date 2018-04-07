/** $chatty_c$ **/
/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.c
 * @section LICENSE
 * ****************************************************************************
 * @author Luca Canessa (516639)											  *
 * 																			  *
 * @copyright \n															  *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 *
 * @section DESCRIPTION
 * Looking overview
 * 
 * This file contains functions to manage requests from clients.
 * The main function is "main ()" where sockets are set, to listen to requests
 * from clients and the socket listener that accepts a connection. When the
 * listener accepts a request, set up a socket to handle requests from a single
 * client. Using a loop and the "poll ()" function, it can parse each socket to
 * determine if a client has sent a request. For each request sent, "main ()"
 * sets up a structure with information about the socket and inserts the
 * request in the threaded task queue with this information. Each request is
 * managed by the "request_handler ()" function. This function analyzes the OP
 * element in the message and with the "switch" command can handle the specific
 * request. The "main ()" function also handles signals using "sig_handler ()",
 * which handles some sent signals.
 * 
 * usage()				:	Function used to print the command, to run the 
 * 							server, in the standard error.
 * 							Requires the pointer to program path.
 * 							Returns nothing
 * 
 * print_statistic()	:	Function used to print the server statistics into
 * 							the log file after update some statistic.
 * 							Requires the pointer to the hash table where saved
 * 							the users and pointer to log file path.
 * 							Returns nothing
 * 
 * configServer()		:	Function used to set the global varabiable that
 * 							sets up the server.
 * 							Requires the pointer to the structure where values
 * 							are saved to configure the server.
 * 							Returns nothing.
 * 
 * cpyMSG()				:	Function used to copy each element of the message
 * 							from one to another.
 * 							Requires the pointer to the destination message and
 * 							the pointer to the source message.
 * 							Returns nothing.
 * 
 * cpyUsr()				:	Function used to copy the active users list into to
 * 							a buffer.
 * 							Requires the pointer to active users queue and
 * 							pointer to a buffer in which to save the list.
 * 							Returns nothing
 * 
 * sendUlist()			:	Function used to set up the message to send with
 * 							the active users.
 * 							Requires the pointer to active users queue, pointer
 * 							to message to send, pointer to user that receive
 * 							the message and pointer to a buffer where save the
 * 							users.
 * 							Returns nothing.
 * 
 * sendtoall()			:	Function used to send a message to each registered
 * 							user. If the user is active, the message is sent
 * 							directly, otherwise it is entered in the history
 * 							and the user can read it whenever he wishes.
 * 							Requires the pointer to the message to be sent and
 * 							the pointer to the hash table where the users were
 * 							saved.
 * 							Returns an integer: if equal to 0 then there were 
 * 							no errors, otherwise some users may not have 
 * 							received the message.
 * 
 * clear_chat()			:	Function used to erase each message in history chat
 * 							Requires the pointer to history chat.
 * 							Return nothing.
 * 
 * rename_f()			:	Function used to rename the file path that will be
 * 							received. Check if other files have the same name,
 * 							in this case it adds a number at the end that
 * 							corresponds to the n-th file that will be saved.
 * 							Requires the pointer to the original file path.
 * 							Returns the pointer to the new path.
 * 
 * request_handler()	:	Function used to manage the requests sent from the
 * 							clients. It reads the message sent and select the
 * 							operation to do according to the "OP" message item.
 * 							The "OP" codes are setted in the file "ops.h".
 *							
 *							REGISTER_OP: registering an user
 *							CONNECT_OP: connecting an user
 *							POSTTXT_OP: receiving a message txt and resending it
 *							POSTTXTALL_OP: receiving and sending it to every user a txt message
 *							POSTFILE_OP: receiving a file
 *							GETFILE_OP: sending a file
 *							GETPREVMSGS_OP: sending the history chat
 *							USRLIST_OP: sending a active user list
 *							UNREGISTER_OP: deregistering an user
 *							DISCONNECT_OP: deactiveting an user
 *							CREATEGROUP_OP: creating a group
 *							ADDGROUP_OP: adding an user to a group
 *							DELGROUP_OP: removing an user from a group or delete the group
 *							GETPREVGROUPMSGS_OP: sending the group history chat
 *				
 *							Requires the pointer to a structure where socket 
 *							information is saved.
 *							Returns nothing.
 *
 * sig_handler()		:	Function to manage the signals: 
 * 								SIGINT
 * 								SIGTERM
 * 								SIGQUIT
 * 								SIGPIPE
 * 								SIGUSR1 
 * 							Requires signal code.
 * 							Returns nothing.
 */



/******************************************************************************
                                   HEADER
******************************************************************************/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <dirent.h>

#include "src/chatty.h"
#include <src/stats.h>
#include <src/queue.h>
#include <src/signup.h>
#include <src/ops.h>
#include <src/pool.h>
#include <src/connections.h>
#include <src/hashtable.h>
#include <src/config.h>


pthread_mutex_t main_l = PTHREAD_MUTEX_INITIALIZER;	///< mutex to block some data managed into this file

static volatile int keepRUN = 1;  ///< global variable to know when the signal is sent to shutdown the server
static volatile int printLOG = 0; ///< global variable to know when the signal is sent to print the log 



/******************************************************************************
							STRUCT AND TYPEDEF
******************************************************************************/
/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0, PTHREAD_MUTEX_INITIALIZER }; //#mutex,#registered user,#client online,#delivered mes,#to be delivered message,#delivered files, #to be delivered files, #errors messages



/**
 *	\typedef		rq_arg
 *	\struct			struct arg_rq_hand
 *	\brief			Structure to save the socket info, and info to manage the requests
 *	\var	users	Pointer to hash table where saved the users
 *	\var	con		Pointer to 'poll' structure
 *	\var	pool	Pointer to thread pool
 *	\var	i_fd	Original socket FD value
 *	\var	fd		Socket FD value
 */
typedef struct arg_rq_hand
{
	hashtable_t *users;
	struct pollfd *con;
	threadpool_t *pool;
	int i_fd;
	int fd;
}rq_arg;



/**
 * @struct			struct of_arg
 * @brief			Structure used to manage file to send
 * @var		path	Pointer to file path
 * @var		f		Pointer to file descriptor to opened file
 */
struct of_arg
{
	char *path;
	FILE *f;
};



/******************************************************************************
                                FUNCTIONS
******************************************************************************/
/**
 * @function					usage
 * @breif				Function to print the correct command that runs the server
 * @param	progname	Pointer to server path
 */
static void usage(const char *progname)
{
    fprintf( stderr, "Il server va lanciato con il seguente comando:\n" );
    fprintf( stderr, "  %s -f <conffile>\n", progname );
}



/**
 * @function					print_statistic
 * @brief				Function to update structure items of statistic log and save
 * 						this log
 * @param	usr			Pointer to the hash table where users are saved
 * @param 	FileName	Pointer to the log path
 */
void print_statistic( hashtable_t *usr, char *FileName )
{
	FILE *stats;

	if( (stats = fopen( FileName, "a+" )) == NULL )
		{
			perror( "fopen()" );
		}

	int r = 0;

		pthread_mutex_lock( &usr->ht_lock );
			chattyStats.nonline = usr->active_user->queue_len;
			chattyStats.nusers = usr->reg_users;
		pthread_mutex_unlock( &usr->ht_lock );

		r = printStats( stats );

	fclose( stats );

	if( r != 0 )
		{
			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
			fprintf( stderr, "Some problems to write on file '%s'.\n", FileName );
		}
}



/**
 * @function				configServer
 * @brief			Function to update the server configuration variables
 * @param	config	Pointer to the structure in which the variables are allocated
 */
void configServer( struct s_conf *config )
{
	//set socket path
	strcpy( SOCKNAME, config->UnixPath );

	//set maximum number of connection
	_MAX_CONN = config->MaxConnections;
	//set number of thread in pool
	_THREADn = config->ThreadsInPool;

	//set the max size of file that can be sent
	_FILESIZE = config->MaxFileSize * 1024;

	//set the max size of message that can be sent
	_MSGSIZE = config->MaxMsgSize;

	//set directory where save files
	strcpy( Dir, config->DirName );

	//set the maximum number of messages to be saved
	_MAX_HIST = config->MaxHistMsgs;
}



/**
 * @function					copyMSG
 * @brief				Function used to copy the elements of a message from one to another
 * @param	msg_dest	Pointer to destination message
 * @param	msg_src		Pointer to source message
 */
void copyMSG( message_t *msg_dest, message_t *msg_src )
{
	memcpy( &msg_dest->hdr.op, &msg_src->hdr.op, sizeof( msg_dest->hdr.op ) );
	memcpy( msg_dest->hdr.sender, msg_src->hdr.sender, sizeof( msg_dest->hdr.sender ) );
	memcpy( &msg_dest->data.hdr.len, &msg_src->data.hdr.len, sizeof( msg_dest->data.hdr.len ) );
	memcpy( msg_dest->data.hdr.receiver, msg_src->data.hdr.receiver, sizeof( msg_dest->data.hdr.receiver ) );
	memcpy( msg_dest->data.buf, msg_src->data.buf, msg_dest->data.hdr.len );
}



/**
 * @function				cpyUsr
 * @brief			Function used to copy the active users from the queue to a buffer
 * @param	usrs	Pointer to the active users' queue
 */
void cpyUsr( queue_t *usrs, char *buf )
{
	node_t *tmp_n = usrs->head;
	int offset = 0;
	user_t *tmp_u = NULL;

	while( tmp_n != NULL )
	{
		tmp_u = tmp_n->ptr;
		strncpy( buf + offset, tmp_u->nickname, MAX_NAME_LENGTH );
		offset += MAX_NAME_LENGTH;
		buf[offset] = '\0';
		offset += 1;
		tmp_n = tmp_n->next;
	}
}



/**
 * @function				sendUlist
 * @brief			Function used to set the message to send to user with active user list
 * @param	u		Pointer to users queue
 * @param	msg		Pointer to the message to be set
 * @param	me		Pointer to the user who must receive the message
 * @param	buff	Pointer to the buffer to insert users
 */
void sendUlist( queue_t *u, message_t *msg, user_t *me, char *buff )
{
	int size_buf = 0;

	size_buf = u->queue_len * ( MAX_NAME_LENGTH + 1 );
	buff = (char *)malloc( (size_buf) * sizeof( char ) );

	pthread_mutex_lock( &u->queue_lock );
		cpyUsr( u, buff );
	pthread_mutex_unlock( &u->queue_lock );

	setData( &msg->data, me->nickname, buff, size_buf );
}



/**
 * @function				sendtoall
 * @brief			Function to send a message to all registered users
 * @param	new 	Pointer to the message to be sent
 * @param	users	Pointer to the hash table where users were saved
 * @return	err		Integer value: if equal to 0 then no errors have occurred 
 * 					otherwise if less than 0 there may have been errors
 */
int sendtoall( message_t *new, hashtable_t *users )
{
	int err = 0;
	for( int i = 0; i < _MAX_CONN; i++ )
		{
			user_t *recvr = users->users[i].user;
			if( recvr != NULL )
				{
					if( strcmp( recvr->nickname, new->hdr.sender ) != 0 )
						{
							message_t *new_m = (message_t *)malloc( sizeof( message_t ) );
							new_m->data.buf = (char *)malloc( new->data.hdr.len * sizeof( char ) );
							copyMSG( new_m, new );

							if( push( recvr->chats, new_m ) == 0 )
								{
									if( recvr->fd_online != -1 )
										{
											if( sendHeader(	recvr->fd_online, &new_m->hdr ) > 0 )
												{
													if( sendData( recvr->fd_online, &new_m->data ) > 0 )
														{
															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.ndelivered += 1;
															pthread_mutex_unlock( &chattyStats.statLock );

															while( recvr->chats->queue_len > _MAX_HIST && recvr->alrdy_read == 1 )
																{
																	message_t *rm = pull( recvr->chats );
																	free( rm->data.buf );
																	free( rm );
																}
														}
													else
														{
															recvr->alrdy_read = 0;

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nnotdelivered += 1;
																chattyStats.nerrors += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
												}
											else
												{
													recvr->alrdy_read = 0;

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nnotdelivered += 1;
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}
										}
									else
										{
											recvr->alrdy_read = 0;

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nnotdelivered += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}

								}
							else
								{
									err -= 1;

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nnotdelivered += 1;
									pthread_mutex_unlock( &chattyStats.statLock );
								}
						}
					if( users->users[i].collision != NULL )
						{
							node_t *tmp = users->users[i].collision->head;
							user_t *us = NULL;

							while( tmp != NULL && tmp->ptr != NULL )
								{
									us = tmp->ptr;
									if( strcmp( us->nickname, new->hdr.sender) != 0 )
										{
											message_t *new_m = (message_t *)malloc( sizeof( message_t ) );
											new_m->data.buf = ( char * )malloc( new->data.hdr.len * sizeof( char ) );
											copyMSG( new_m, new );

											if( push( us->chats, new_m ) == 0 )
												{
													if( us->fd_online != -1 )
														{
															if( sendHeader( us->fd_online, &new_m->hdr ) > 0 )
																{
																	if( sendData( us->fd_online, &new_m->data ) > 0 )
																		{
																			pthread_mutex_lock( &chattyStats.statLock );
																				chattyStats.ndelivered += 1;
																			pthread_mutex_unlock( &chattyStats.statLock );

																			while( us->chats->queue_len > _MAX_HIST && us->alrdy_read == 1 )
																				{
																					message_t *rm = pull( us->chats );
																					free( rm->data.buf );
																					free( rm );
																				}
																		}
																	else
																		{
																			us->alrdy_read = 0;

																			pthread_mutex_lock( &chattyStats.statLock );
																				chattyStats.nnotdelivered += 1;
																				chattyStats.nerrors += 1;
																			pthread_mutex_unlock( &chattyStats.statLock );
																		}
																}
															else
																{
																	us->alrdy_read = 0;

																	pthread_mutex_lock( &chattyStats.statLock );
																		chattyStats.nnotdelivered += 1;
																		chattyStats.nerrors += 1;
																	pthread_mutex_unlock( &chattyStats.statLock );
																}
														}
													else
														{
															us->alrdy_read = 0;

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nnotdelivered += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
												}
											else
												{
													err -= 1;

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nnotdelivered += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}
										}
									tmp = tmp->next;
								}
						}
				}

		}
	if( new )
		{
			if( new->data.buf )
				{
					free( new->data.buf );
				}
			free( new );
		}
	return err;
}



/**
 * @function				clear_chat
 * @brief			Function to clear the messages chat from user history queue
 * @param	chat	Pointer to the user's message queue
 */
void clear_chat( queue_t *chat )
{
	message_t *tmsg;
	while( chat != NULL )
		{
			tmsg = pull( chat );

			if( tmsg != NULL )
				{
					if( tmsg->data.buf )
						{
							free( tmsg->data.buf );
						}
					free( tmsg );
				}
			else
				{
					destroy_queue( chat );
					break;
				}
		}
}



/**
 * @function				rename_f
 * @brief			Function to change the path of the file to be received. 
 * 					If the file already exists, it also updates the name.
 * @param	old		Pointer to the path of the original file received
 * @return	new		Pointer to the new file path
 */
char *rename_f( char *old )
{
	struct stat		ds;
    struct stat 	fs;
    char 			*new = NULL;
    int 			dirlen = strlen( Dir );
    int 			oldlen = strlen( old );
    int 			st_name = oldlen;
    int 			e = 0;
	int				slash = 0;

    // DIR *chDir = opendir( Dir );
    // if( !chDir )
    //     {
    //         mkdir( Dir, 0775 );
    //     }
	int DS = stat( Dir, &ds );
	if( DS == -1 )
		{
			mkdir( Dir, 0755 );
		}
	else
		{
			if( DS == 0 && !S_ISDIR( ds.st_mode ) )
				{
					mkdir( Dir, 0755 );
				}
		}

    while( old[st_name] != '/' && st_name != 0 )
        {
            if( old[st_name] == '.' && e == 0 )
                {
                    e = st_name;
                }
            st_name -= 1;
        }

    if( e != 0 )
        {
            if( old[st_name] == '/' && Dir[dirlen-1] == '/' )
                {
                    st_name++;
                }
            if( old[st_name] != '/' && Dir[dirlen-1] != '/' )
                {
                    slash = 1;
                }

            int cn = oldlen - st_name;
            int se = oldlen - e;
            int sn = cn - se;

            char name[sn+1];
            strncpy( name, old+st_name, sn );
            name[sn] = '\0';

            new = ( char * )malloc( (dirlen+slash+sn+se+1) * sizeof( char ) );
            if( slash )
				{
					sprintf( new, "%s/%s", Dir, old+st_name );
				}
			else
				{
					sprintf( new, "%s%s", Dir, old+st_name );
				}

            int already = 0;
            while( stat( new, &fs ) != -1 )
                {
                    already ++;
                    char nnew[17];
                    sprintf( nnew, "_%d", already );
                    int lennnew = strlen( nnew );

					free( new );
					new = ( char * )malloc( dirlen+slash+sn+se+lennnew+1 * sizeof( new ) );

					if( slash )
						{
							sprintf( new, "%s/%s%s%s", Dir, name, nnew, old+e);
						}
					else
						{
							sprintf( new, "%s%s%s%s", Dir, name, nnew, old+e);
						}

                }
        }
    else
        {
            if( old[st_name] == '/' && Dir[dirlen-1] == '/' )
                {
                    st_name++;
                }
            if( old[st_name] != '/' && Dir[dirlen-1] != '/' )
                {
					slash = 1;
                }

            int sn = oldlen - st_name;

            char name[sn+1];
            // name[sn] = '\0';
            strncpy( name, old+st_name, sn );
            name[sn] = '\0';

            new = ( char * )malloc( (dirlen+slash+sn+1) * sizeof( char ) );

			if( slash )
				{
					sprintf( new, "%s/%s", Dir, old+st_name );
				}
			else
            	{
					sprintf( new, "%s%s", Dir, old+st_name );
				}


            int already = 0;
            while( stat( new, &fs ) != -1 )
                {
                    already++;
                    char nnew[17];
                    sprintf( nnew, "_%d", already );
                    int lennnew = strlen( nnew );

					free( new );
					new = ( char * )malloc( dirlen+slash+sn+lennnew+1 * sizeof( char ) );


					if( slash )
						{
							sprintf( new, "%s/%s%s", Dir, name, nnew );
						}
					else
						{
							sprintf( new, "%s%s%s", Dir, name, nnew );
						}

                }
       }
    // closedir( chDir );

    return new;
}



/**
 * @function				request_handler
 * @brief			Function to manage all clients requests
 * @param	args	Pointer to the data structure where the information of the request to
 * 					be managed and the information of the socket used are saved
 */
void requests_handler( void *args )
{
	// EXPLODE THE ARGUMENTS PASSED //
	rq_arg 			*mycon = (rq_arg *)args;
	hashtable_t 	*users = mycon->users;
	struct pollfd 	*con = mycon->con;
	int 			i = mycon->i_fd;
	int 			c_fd = mycon->fd;


	// SET LOCAL VARIABLES //
	message_t 	msg;
	user_t 		*me = NULL;
	char 		*buff = NULL;
	bool		STOP = false;

	// START HANDLING //
	//CLIENT CLOSES SOCKET OR HAS FINISHED SENDING THE MESSAGE
	if( readMsg( c_fd, &msg ) <= 0 )
		{
// printf( "IN RDMSG\n" );
			int ack = 0;
			node_t *rm = users->active_user->head;
			user_t *u_rm = NULL;
			// pthread_mutex_lock( &users->ht_lock );
				while( rm != NULL && !ack )
					{
						u_rm = rm->ptr;
						if( u_rm != NULL )
							{
								if( u_rm->fd_online == c_fd )
									{
										u_rm->fd_online = -1;
										u_rm->alrdy_read = 0;
										// pthread_mutex_unlock( &users->ht_lock );
										ack = remove_node( users->active_user, rm );
										con[i].fd = -1;
										break;
									}
							}
						rm = rm->next;
					}
			// pthread_mutex_unlock( &users->ht_lock );
			if( ack == 0 )
				{
					setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
					pthread_mutex_lock( &chattyStats.statLock );
						chattyStats.nerrors += 1;
					pthread_mutex_unlock( &chattyStats.statLock );

					con[i].fd = c_fd;
					sendHeader( c_fd, &msg.hdr );
				}
// printf( "OUT RDMSG\n\n" );
		}
	else
		{
			// MANAGE REQUEST DEPENDING ON THE 'OP'
			switch( msg.hdr.op )
				{
		//===================================================================//
					case REGISTER_OP :///< REGISTER AN USER
						{
// printf( "IN REG\n" );
						 	int ack_reg = 0;
							ack_reg = checkin( users, msg.hdr.sender );

							//IF THE USER HAS BEEN REGISTERED
							if( ack_reg == true )
								{
									me = connecting( users, msg.hdr.sender );
									if( me != NULL ) //if the user is logged on
										{
											me->fd_online = c_fd;
											push( users->active_user, me );

											//SET USER LIST TO SEND
											sendUlist( users->active_user, &msg, me, buff ); //set the data in this function
											setHeader( &msg.hdr, OP_OK, "ChattyServer" );

											sendHeader( c_fd, &msg.hdr );
											sendData( c_fd, &msg.data );

											if( buff )
												{
													free( buff );
												}
											if( msg.data.buf )
												{
													free( msg.data.buf );
												}
										}
									//IF THE USER IS NOT LOGGED ON
									else
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );

											STOP = true;
										}
								}
							//IF THE USER HAS NOT BEEN REGISTERED
							else
								{
									(ack_reg == false) ? setHeader( &msg.hdr, OP_FAIL, "ChattyServer" ) : setHeader( &msg.hdr, OP_NICK_ALREADY, "ChattyServer" );
									sendHeader( c_fd, &msg.hdr );

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );

									STOP = true;
								}
// printf( "OUT REG\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case CONNECT_OP :///< CONNECT AN USER
						{
// printf( "IN CONN\n" );
							me = connecting( users, msg.hdr.sender );

							if( me != NULL ) //if the user is logged on
								{
									if( users->active_user == NULL )
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );

											STOP = true;
										}
									else
										{
											//CHECK IF USER IS ALREADY ONLINE
											if( me->fd_online == -1 ) //if offline
												{
													me->fd_online = c_fd;
													push( users->active_user, me );

													//SET USER LIST TO SEND
													sendUlist( users->active_user, &msg, me, buff ); //set the data in this function
													setHeader( &msg.hdr, OP_OK, "ChattyServer" );

													sendHeader( c_fd, &msg.hdr );
													sendData( c_fd, &msg.data );

													if( buff )
														{
															free( buff );
														}
													if( msg.data.buf )
														{
															free( msg.data.buf );
														}
												}
											else //if online
												{
													setHeader( &msg.hdr, OP_ALREADY_ONLINE, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );

													STOP = true;
												}
										}
								}
							else
								{
									setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
									sendHeader( c_fd, &msg.hdr );

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );

									STOP = true;
								}
// printf( "OUT CONN\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case USRLIST_OP :///< SEND USER LIST
						{
// printf( "IN LIST\n" );
							me = connecting( users, msg.hdr.sender );

							//SET USER LIST TO SEND
							sendUlist( users->active_user, &msg, me, buff ); //set the data in this function
							setHeader( &msg.hdr, OP_OK, "ChattyServer" );

							sendHeader( c_fd, &msg.hdr );
							sendData( c_fd, &msg.data );

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
// printf( "OUT LIST\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case DISCONNECT_OP :///< DISCONNECT AN USER
						{
							me = connecting( users, msg.hdr.sender );

							if( me != NULL )
								{
									me->fd_online = -1;
									node_t 	*rm = users->active_user->head;
									user_t 	*u_rm = NULL;
									if( rm != NULL )
										{
											u_rm = rm->ptr;
										}

									//PUT USER OFFLINE
									while( rm != users->active_user->head && strcmp( u_rm->nickname, me->nickname ) != 0 )
										{
											rm = rm->next;
											u_rm = rm->ptr;
										}

									int ack = 0;
									if( rm != NULL )
										{
											ack = remove_node( users->active_user, rm );
										}
									if( ack == 1 )
										{
											setHeader( &msg.hdr, OP_OK, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );
											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}
							else
								{
									setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
									sendHeader( c_fd, &msg.hdr );

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );
								}

								if( buff )
									{
										free( buff );
									}
								if( msg.data.buf )
									{
										free( msg.data.buf );
									}
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case UNREGISTER_OP :///< UNREGISTER AN USER
						{
// printf( "IN UNREG\n" );
							me = connecting( users, msg.hdr.sender );

							if( me != NULL )
								{
									pthread_mutex_lock( &main_l );
										me->fd_online = -1;
										con[i].fd = -1;
									pthread_mutex_unlock( &main_l );

									node_t *rm_n = users->active_user->head;
									user_t *u_rm = NULL;

									pthread_mutex_lock( &users->active_user->queue_lock );
									//PUT USER OFFLINE
										while( rm_n != NULL )
											{
												u_rm = rm_n->ptr;
												if( strcmp(u_rm->nickname, me->nickname) == 0 )
													{
														break;
													}
												else
													{
														rm_n = rm_n->next;
													}
											}
									pthread_mutex_unlock( &users->active_user->queue_lock );

									if( rm_n != NULL && remove_node( users->active_user, rm_n ) == 1 )
										{
											clear_chat( me->chats );

											if( delete( users, me->nickname ) == true )
												{

													setHeader( &msg.hdr, OP_OK, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );
												}
											else
												{
													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}

											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}
							else
								{
									setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
									sendHeader( c_fd, &msg.hdr );

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );
								}

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
// printf( "OUT UNREG\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case CREATEGROUP_OP :///< CREATE A GROUP
						{
							char 			*gname = msg.data.hdr.receiver;
							group_chat_t 	*g = NULL;

							me = connecting( users, msg.hdr.sender );

							pthread_mutex_lock( &users->ht_lock );
								g = searchGroup( users, gname );
							pthread_mutex_unlock( &users->ht_lock );

							if( me != NULL && g == NULL )
								{
									pthread_mutex_lock( &users->ht_lock );
										bool g_ack = addGroup( users, gname );
									pthread_mutex_unlock( &users->ht_lock );

									if( g_ack == false )
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
									else
										{
											pthread_mutex_lock( &users->ht_lock );
												g = searchGroup( users, gname );
											pthread_mutex_unlock( &users->ht_lock );

											if( g != NULL )
												{
													strcpy( g->creator, me->nickname );
													if( push( g->participants, me ) == 0 && push( me->mygroup, g ) == 0 )
														{
															setHeader( &msg.hdr, OP_OK, "ChattyServer" );
															sendHeader( c_fd, &msg.hdr );
														}
													else
														{
															removingGroup( users, gname );
															setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
															sendHeader( c_fd, &msg.hdr );
														}
												}
											else
												{
													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}
										}
								}
							else
								{
									if( me == NULL )
										{
											setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );

											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_NICK_AVAILABLE, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case ADDGROUP_OP :///< ADDITION OF A USER TO A GROUP
						{
							char 			*gname = msg.data.hdr.receiver;
							group_chat_t 	*g = NULL;

							me = connecting( users, msg.hdr.sender );

							pthread_mutex_lock( &users->ht_lock );
								g = searchGroup( users, gname );
							pthread_mutex_unlock( &users->ht_lock );

							if( me != NULL && g != NULL )
								{
									if( push( g->participants, me ) == 0 && push( me->mygroup, g ) == 0 )
										{
											setHeader( &msg.hdr, OP_OK, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );
										}
									else
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}
							else
								{
									if( me == NULL )
										{
											setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );
										}
									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );
								}

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case DELGROUP_OP :///< DELETE A GROUP OR REMOVE A USER
						{
							char 			*gname = msg.data.hdr.receiver;
							group_chat_t 	*g = NULL;

							me = connecting( users, msg.hdr.sender );

							pthread_mutex_lock( &users->ht_lock );
								g = searchGroup( users, gname );
							pthread_mutex_unlock( &users->ht_lock );

							if( me != NULL && g != NULL )
								{
									if( strcmp( me->nickname, g->creator ) == 0 )//if me is creator
										{
											node_t *tmp = me->mygroup->head;
											while( tmp != NULL )//remove group from mygroup queue
											{
												group_chat_t *tg = tmp->ptr;
												if( strcmp( tg->chat_title, gname) == 0 )
													{
														remove_node( me->mygroup, tmp );
														break;
													}
												else
													{
														tmp = tmp->next;
													}
											}

											pthread_mutex_lock( &users->ht_lock );
												bool r = removingGroup( users, gname );
											pthread_mutex_unlock( &users->ht_lock );

											if( r == true ) //if prev operation was completed then remove group definitely
												{
													setHeader( &msg.hdr, OP_OK, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );
												}
											else
												{
													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}
										}
									else//if me is not creator
										{
											node_t *tmp = me->mygroup->head;
											int ack = 0;

											while( tmp != NULL && !ack )
												{
													group_chat_t *tg = tmp->ptr;
													if( strcmp( tg->chat_title, gname) == 0 )
														{
															remove_node( me->mygroup, tmp );
															ack = 1;
														}
													else
														{
															tmp = tmp->next;
														}
												}

											if( ack == 1 )
												{
													tmp = g->participants->head;
													while( tmp != NULL )
														{
															user_t *myname = tmp->ptr;
															if( strcmp( myname->nickname, me->nickname ) == 0 )
																{
																	remove_node( g->participants, tmp );

																	setHeader( &msg.hdr, OP_OK, "ChattyServer" );
																	sendHeader( c_fd, &msg.hdr );

																	break;
																}
															else
																{
																	tmp = tmp->next;
																}
														}

													if( tmp == NULL )
														{
															setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
															sendHeader( c_fd, &msg.hdr );

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nerrors += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
												}
											else
												{
													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}
										}
								}
							else
								{
									if( me == NULL )
										{
											setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case POSTTXT_OP :///< SEND MESSAGE RECEIVED TO A RECEIVER USER OR GROUP
						{
// printf( "IN PT\n" );
							pthread_mutex_lock( &users->ht_lock );
								group_chat_t *g = searchGroup( users, msg.data.hdr.receiver );
							pthread_mutex_unlock( &users->ht_lock );

							user_t *recvr = connecting( users, msg.data.hdr.receiver );
							me = connecting( users, msg.hdr.sender );

							if( me != NULL && (recvr != NULL || g != NULL ) )
								{

									if( msg.data.hdr.len > _MSGSIZE )
										{
											setHeader( &msg.hdr, OP_MSG_TOOLONG, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );

											sendHeader( c_fd, &msg.hdr );
										}
									else
										{
											message_t *new = (message_t *)malloc( sizeof( message_t ) );
											new->data.buf = (char *)malloc( msg.data.hdr.len * sizeof( char ) );
											copyMSG( new, &msg );

											if( g != NULL )
												{
													int flag = 0;

													// CHECK IF USER 'me' IS A PARTECIPANT OF GROUP 'g'
													pthread_mutex_lock( &g->participants->queue_lock );
														node_t *n_u = g->participants->head;
														while( n_u != NULL && !flag )
															{
																user_t *ut = n_u->ptr;
																if( strcmp( ut->nickname, me->nickname ) == 0 )
																	{
																		flag = 1;
																	}
																n_u = n_u->next;
															}
													pthread_mutex_unlock( &g->participants->queue_lock );


													if( flag )
														{
															char sendr[MAX_NAME_LENGTH+MAX_NAME_LENGTH];
															strcpy( sendr, g->chat_title );
															strcat( sendr, " @ " );
															strcat( sendr, me->nickname );

															setHeader( &new->hdr, TXT_MESSAGE, sendr );

															if( push( g->messages, new ) == 0 )
																{
																	setHeader( &msg.hdr, OP_OK, "ChattyServer" );
																	sendHeader( c_fd, &msg.hdr );

																	node_t *tmp_s = g->participants->head;
																	while( tmp_s != NULL )
																		{
																			message_t *new_m = ( message_t * )malloc( sizeof( message_t ) );
																			new_m->data.buf = ( char * )malloc( new->data.hdr.len+1 * sizeof( char ) );
																			copyMSG( new_m, new );

																			user_t *u = tmp_s->ptr;
																			if( push( u->chats, new_m ) == 0 )
																				{
																					if( u->fd_online != -1 )
																						{
																							if( sendHeader( u->fd_online, &new->hdr ) > 0 )
																								{
																									if( sendData( u->fd_online, &new->data ) > 0 )
																										{
																											pthread_mutex_lock( &chattyStats.statLock );
																												chattyStats.ndelivered += 1;
																											pthread_mutex_unlock( &chattyStats.statLock );

																											while( u->chats->queue_len > _MAX_HIST && u->alrdy_read == 1 )
																												{
																													message_t *rm = pull( recvr->chats );
																													free( rm->data.buf );
																													free( rm );
																												}
																										}
																									else
																										{
																											u->alrdy_read = 0;
																											pthread_mutex_lock( &chattyStats.statLock );
																												chattyStats.nnotdelivered += 1;
																												chattyStats.nerrors += 1;
																											pthread_mutex_unlock( &chattyStats.statLock );
																										}
																								}
																							else
																								{
																									u->alrdy_read = 0;
																									pthread_mutex_lock( &chattyStats.statLock );
																										chattyStats.nnotdelivered += 1;
																										chattyStats.nerrors += 1;
																									pthread_mutex_unlock( &chattyStats.statLock );
																								}
																						}
																					else
																						{
																							u->alrdy_read = 0;

																							pthread_mutex_lock( &chattyStats.statLock );
																								chattyStats.nnotdelivered += 1;
																							pthread_mutex_unlock( &chattyStats.statLock );
																						}
																				}
																			tmp_s = tmp_s->next;
																		}
																}
															else
																{
																	setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
																	sendHeader( c_fd, &msg.hdr );

																	pthread_mutex_lock( &chattyStats.statLock );
																		chattyStats.nnotdelivered += 1;
																		chattyStats.nerrors += 1;
																	pthread_mutex_unlock( &chattyStats.statLock );

																	STOP = true;
																}
														}
													else
														{
															setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
															sendHeader( c_fd, &msg.hdr );

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nnotdelivered += 1;
																chattyStats.nerrors += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
												}
											else
												{
													setHeader( &new->hdr, TXT_MESSAGE, me->nickname );

													if( push( recvr->chats, new ) == 0 )
														{
															setHeader( &msg.hdr, OP_OK, "ChattyServer" );
															sendHeader( c_fd, &msg.hdr );

															if( recvr->fd_online != -1 )
																{
																	if( sendHeader( recvr->fd_online, &new->hdr ) > 0 )
																		{
																			if( sendData( recvr->fd_online, &new->data ) > 0 )
																				{
																					pthread_mutex_lock( &chattyStats.statLock );
																						chattyStats.ndelivered += 1;
																					pthread_mutex_unlock( &chattyStats.statLock );

																					while( recvr->chats->queue_len > _MAX_HIST && recvr->alrdy_read == 1 )
																						{
																							message_t *rm = pull( recvr->chats );
																							free( rm->data.buf );
																							free( rm );
																						}
																				}
																			else
																				{
																					recvr->alrdy_read = 0;

																					pthread_mutex_lock( &chattyStats.statLock );
																						chattyStats.nnotdelivered += 1;
																						chattyStats.nerrors += 1;
																					pthread_mutex_unlock( &chattyStats.statLock );
																				}
																		}
																	else
																		{
																			recvr->alrdy_read = 0;

																			pthread_mutex_lock( &chattyStats.statLock );
																				chattyStats.nnotdelivered += 1;
																				chattyStats.nerrors += 1;
																			pthread_mutex_unlock( &chattyStats.statLock );
																		}
																}
															else
																{
																	recvr->alrdy_read = 0;

																	pthread_mutex_lock( &chattyStats.statLock );
																		chattyStats.nnotdelivered += 1;
																	pthread_mutex_unlock( &chattyStats.statLock );

																}
														}
													else
														{
															setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
															sendHeader( c_fd, &msg.hdr );

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nnotdelivered += 1;
																chattyStats.nerrors += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}

												}
										}
								}
							else
								{
									if( me == NULL || recvr == NULL )
										{
											if( me == NULL )
												{
													STOP = true;
												}
											setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nnotdelivered += 1;
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
									else
										{
											setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nnotdelivered += 1;
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}
							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
// printf( "OUT PT\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case POSTTXTALL_OP :///< SEND A MESSAGE RECEIVED TO ALL USERS
						{
// printf( "IN PA\n" );
							me = connecting( users, msg.hdr.sender );

							if( me != NULL )
								{
									if( msg.data.hdr.len > _MSGSIZE )
										{
											setHeader( &msg.hdr, OP_MSG_TOOLONG, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
									else
										{
											message_t *new = (message_t *)malloc( sizeof( message_t ) );
											new->data.buf = (char *)malloc( msg.data.hdr.len * sizeof( char ) );
											copyMSG( new, &msg );

											setHeader( &msg.hdr, OP_OK, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											setHeader( &new->hdr, TXT_MESSAGE, me->nickname );

											int ack;
											pthread_mutex_lock( &users->ht_lock );
												ack = sendtoall( new, users );
											pthread_mutex_unlock( &users->ht_lock );

											if( ack != 0 )
												{
													setHeader( &msg.hdr, MSG_NDELIV_SOMEUSERS, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );
												}
										}
								}
							else
								{
									STOP = true;

									setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
									sendHeader( c_fd, &msg.hdr );
								}

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
// printf( "OUT PA\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case POSTFILE_OP :///< SEND FILE RECEIVED TO A RECEIVER USER OR GROUP
						{
// printf( "IN PF\n" );
							user_t *recvr = connecting( users, msg.data.hdr.receiver );
							me = connecting( users, msg.hdr.sender );
                            int _f_tmp = -1;

							pthread_mutex_lock( &users->ht_lock );
								group_chat_t *g = searchGroup( users, msg.data.hdr.receiver );
							pthread_mutex_unlock( &users->ht_lock );

							if( me != NULL && (recvr != NULL || g != NULL ) )
								{
									if( msg.data.hdr.len > _MSGSIZE )
										{
											setHeader( &msg.hdr, OP_MSG_TOOLONG, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
									else
										{
												// OPENING FILE IN CHATTY DIR TO BE WRITTEN
												message_data_t file;
												message_t *new = ( message_t * )calloc(1, sizeof( message_t ) );

                                                char *newf = rename_f( msg.data.buf );
												new->data.buf = (char *)calloc( (strlen( newf )+1), sizeof( char ) );
												copyMSG( new, &msg );

												strcpy( new->data.buf, newf );
												new->data.hdr.len = strlen( newf )+1;

												free( newf );

												if( msg.data.buf )
													{
														free( msg.data.buf );
													}

												if ( (_f_tmp = open( new->data.buf, O_CREAT | O_WRONLY, 0644 )) == -1 )
													{
														perror( "open" );

														pthread_mutex_lock( &chattyStats.statLock );
															chattyStats.nerrors += 1;
															chattyStats.nfilenotdelivered += 1;
														pthread_mutex_unlock( &chattyStats.statLock );

														setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
														sendHeader( c_fd, &msg.hdr );

														break;
													}
											if( g != NULL )
												{
													int flag = 0;

													//FIND USER ON PARTICIPANTS QUEUE
													pthread_mutex_lock( &g->participants->queue_lock );
														node_t *n_u = g->participants->head;
														while( n_u != NULL && !flag )
															{
																user_t *ut = n_u->ptr;
																if( strcmp( ut->nickname, me->nickname ) == 0 )
																	flag = 1;
																n_u = n_u->next;
															}
													pthread_mutex_unlock( &g->participants->queue_lock );

													if( flag )
														{
															char sendr[MAX_NAME_LENGTH+MAX_NAME_LENGTH];
															strcpy( sendr, g->chat_title );
															strcat( sendr, " @ " );
															strcat( sendr, me->nickname );

															setHeader( &new->hdr, FILE_MESSAGE, sendr );

															if( readData( c_fd, &file ) > 0 )
																{
																	if( file.hdr.len > _FILESIZE )
																		{
																			setHeader( &msg.hdr, OP_MSG_TOOLONG, "ChattyServer" );
																			sendHeader( c_fd, &msg.hdr );

																			pthread_mutex_lock( &chattyStats.statLock );
																				chattyStats.nerrors += 1;
																				chattyStats.nfilenotdelivered += 1;
																			pthread_mutex_unlock( &chattyStats.statLock );

																			fsync( _f_tmp );
																			close( _f_tmp );
																			remove( new->data.buf );
																			
																			if( new )
																				{
																					if( new->data.buf )
																						{
																							free( new->data.buf );
																						}
																					free( new );
																				}
																		}
																	else
																		{
																			write( _f_tmp, file.buf, file.hdr.len );
																			fsync( _f_tmp );
																			close( _f_tmp );

																			if( push( g->messages, new ) == 0 )
																				{
                                                                                    setHeader( &msg.hdr, OP_OK, "ChattyServer" );
                                                                                    sendHeader( c_fd, &msg.hdr );

                                                                                    pthread_mutex_lock( &g->participants->queue_lock );
																						node_t *tmp_s = g->participants->head;
																						while( tmp_s != NULL )
																							{
																								message_t *new_m = (message_t *)calloc( 1, sizeof( message_t ) );
																								new_m->data.buf = (char *)calloc( new->data.hdr.len, sizeof( char ) );
																								copyMSG( new_m, new );

																								user_t *u = tmp_s->ptr;
																								if( push( u->chats, new_m ) == 0 )
																									{
																										if( u->fd_online != -1 )
																											{
																												if( sendHeader(	u->fd_online, &new_m->hdr ) > 0 )
																													{
																														if( sendData( u->fd_online, &new_m->data ) > 0 )
																															{
																																pthread_mutex_lock( &chattyStats.statLock );
																																	chattyStats.nfilenotdelivered += 1;
																																	chattyStats.ndelivered += 1;
																																pthread_mutex_unlock( &chattyStats.statLock );

																																while( u->chats->queue_len > _MAX_HIST && u->alrdy_read == 1 )
																																	{
																																		message_t *rm = pull( u->chats );
																																		if( rm->data.buf != NULL )
																																			free( rm->data.buf );
																																		free( rm );
																																	}
																															}
																														else
																															{
																																u->alrdy_read = 0;

																																pthread_mutex_lock( &chattyStats.statLock );
																																	chattyStats.nerrors += 1;
																																	chattyStats.nfilenotdelivered += 1;
																																pthread_mutex_unlock( &chattyStats.statLock );
																															}
																													}
																												else
																													{
																														u->alrdy_read = 0;

																														pthread_mutex_lock( &chattyStats.statLock );
																															chattyStats.nerrors += 1;
																															chattyStats.nfilenotdelivered += 1;
																														pthread_mutex_unlock( &chattyStats.statLock );
																													}
																											}
																										else
																											{
																												u->alrdy_read = 0;

																												pthread_mutex_lock( &chattyStats.statLock );
																													chattyStats.nfilenotdelivered += 1;
																												pthread_mutex_unlock( &chattyStats.statLock );
																											}
																									}
																								else
																									{
																										if( new_m )
																											{
																												if( new_m->data.buf )
																													{
																														free( new_m->data.buf );
																													}
																												free( new_m );
																											}

																										pthread_mutex_lock( &chattyStats.statLock );
																											chattyStats.nerrors += 1;
																											chattyStats.nfilenotdelivered += 1;
																										pthread_mutex_unlock( &chattyStats.statLock );
																									}
																								tmp_s = tmp_s->next;
																							}
																					pthread_mutex_unlock( &g->participants->queue_lock );
																				}
																			else
																				{
																					if( new )
																						{
																							if( new->data.buf )
																								{
																									free( new->data.buf );
																								}
																							free( new );
																						}
																						
																					setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
                                                                                    sendHeader( c_fd, &msg.hdr );

																					pthread_mutex_lock( &chattyStats.statLock );
																						chattyStats.nerrors += 1;
																						chattyStats.nfilenotdelivered += 1;
																					pthread_mutex_unlock( &chattyStats.statLock );
																				}
																		}
																}
															else
																{
																	if( new )
																		{
																			if( new->data.buf )
																				{
																					free( new->data.buf );
																				}
																			free( new );
																		}
																		
																	setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
                                                                    sendHeader( c_fd, &msg.hdr );

																	pthread_mutex_lock( &chattyStats.statLock );
																		chattyStats.nerrors += 1;
																		chattyStats.nfilenotdelivered += 1;
																	pthread_mutex_unlock( &chattyStats.statLock );
																}
															if( file.buf )
																{
																	free( file.buf );
																}
														}
													else
														{
															if( new )
																{
																	if( new->data.buf )
																		{
																			free( new->data.buf );
																		}
																	free( new );
																}
															setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
                                                            sendHeader( c_fd, &msg.hdr );

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nerrors += 1;
																chattyStats.nfilenotdelivered += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
												}
											else
												{
													setHeader( &new->hdr, FILE_MESSAGE, me->nickname );

													if( readData( c_fd, &file ) > 0 )
														{
															if( file.hdr.len > _FILESIZE )
																{
																	setHeader( &msg.hdr, OP_MSG_TOOLONG, "ChattyServer" );
																	sendHeader( c_fd, &msg.hdr );

																	pthread_mutex_lock( &chattyStats.statLock );
																		chattyStats.nerrors += 1;
																		chattyStats.nfilenotdelivered += 1;
																	pthread_mutex_unlock( &chattyStats.statLock );

																	fsync( _f_tmp );
																	close( _f_tmp );
																	remove( new->data.buf );
																	
																	if( new )
																		{
																			if( new->data.buf )
																				{
																					free( new->data.buf );
																				}
																			free( new );
																		}
																}
															else
																{
																	write( _f_tmp, file.buf, file.hdr.len );
																	fsync( _f_tmp );
																	close( _f_tmp );

																	if( push( recvr->chats, new ) == 0 )
																		{
																			setHeader( &msg.hdr, OP_OK, "ChattyServer" );
                                                                            sendHeader( c_fd, &msg.hdr );

																			if( recvr->fd_online != -1 )
																				{
																					if( sendHeader( recvr->fd_online, &new->hdr ) > 0 )
																						{
																							if( sendData( recvr->fd_online, &new->data ) > 0 )
																								{
                                                                                                    pthread_mutex_lock( &chattyStats.statLock );
																										chattyStats.ndelivered += 1;
																										chattyStats.nfilenotdelivered -= 1;
																									pthread_mutex_unlock( &chattyStats.statLock );

																									while( recvr->chats->queue_len > _MAX_HIST && recvr->alrdy_read == 1 )
																										{
																											message_t *rm = pull( recvr->chats );
																											free( rm->data.buf );
																											free( rm );
																										}
																								}
																							else
																								{
																									recvr->alrdy_read = 0;

																									pthread_mutex_lock( &chattyStats.statLock );
																										chattyStats.nfilenotdelivered += 1;
																										chattyStats.nerrors += 1;
																									pthread_mutex_unlock( &chattyStats.statLock );
																								}
																						}
																					else
																						{
																							recvr->alrdy_read = 0;

																							pthread_mutex_lock( &chattyStats.statLock );
																								chattyStats.nfilenotdelivered += 1;
																								chattyStats.nerrors += 1;
																							pthread_mutex_unlock( &chattyStats.statLock );
																						}
																				}
																			else
																				{
																					recvr->alrdy_read = 0;

																					pthread_mutex_lock( &chattyStats.statLock );
																						chattyStats.nfilenotdelivered += 1;
																					pthread_mutex_unlock( &chattyStats.statLock );
																				}
																		}
																	else
																		{
																			setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
																			sendHeader( c_fd, &msg.hdr );

																			pthread_mutex_lock( &chattyStats.statLock );
																				chattyStats.nfilenotdelivered += 1;
																				chattyStats.nerrors += 1;
																			pthread_mutex_unlock( &chattyStats.statLock );

																			fsync( _f_tmp );
																			close( _f_tmp );
																			remove( new->data.buf );

																			if( new )
																				{
																					if( new->data.buf )
																						{
																							free( new->data.buf );
																						}
																					free( new );
																				}																		
																		}
																}
														}
													else
														{
															setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
                                                            sendHeader( c_fd, &msg.hdr );

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nfilenotdelivered += 1;
																chattyStats.nerrors += 1;
															pthread_mutex_unlock( &chattyStats.statLock );

															fsync( _f_tmp );
															close( _f_tmp );
															remove( new->data.buf );

															if( new )
																{
																	if( new->data.buf )
																		{
																			free( new->data.buf );
																		}
																	free( new );
																}
														}
													if( file.buf )
														{
															free( file.buf );
														}
												}
										}
								}
							else
								{
									if( me == NULL || recvr == NULL )
										{
											if( me == NULL )
												{
													STOP = true;
												}

											setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
                                            sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nfilenotdelivered += 1;
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
									else
										{
											setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
                                            sendHeader( c_fd, &msg.hdr );

                                            pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nfilenotdelivered += 1;
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}

							if( buff )
								{
									free( buff );
								}
// printf( "OUT PF\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case GETPREVMSGS_OP :///< SEND USER CHAT HISTORY
						{
// printf( "IN GP\n" );
							me = connecting( users, msg.hdr.sender );

							if( me != NULL )
								{
										buff = ( char * )malloc( sizeof( size_t ) );
										memcpy( buff, &me->chats->queue_len, sizeof( int ) );

										setData( &msg.data, me->nickname, buff, sizeof( int ) );
										setHeader( &msg.hdr, OP_OK, "ChattyServer" );

										sendHeader( c_fd, &msg.hdr );
										sendData( c_fd, &msg.data );

										free( buff );

									pthread_mutex_lock( &me->chats->queue_lock );
										node_t *t = me->chats->head;
										message_t *mt = NULL;

										while( t != NULL )
											{
												if( t->ptr == NULL )
													{
														break;
													}
												else
													{
														mt = t->ptr;

														sendHeader( c_fd, &mt->hdr );
														sendData( c_fd, &mt->data );

														t = t->next;

														if( me->alrdy_read == 0 )
															{
															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.ndelivered += 1;
																chattyStats.nnotdelivered -= 1;
															pthread_mutex_unlock( &chattyStats.statLock );
															}
													}
											}
										me->alrdy_read = me->alrdy_read == 0 ? 1 : me->alrdy_read;
									pthread_mutex_unlock( &me->chats->queue_lock );

									while( me->chats->queue_len > _MAX_HIST )
										{
											mt = pull( me->chats );
											if( mt->data.buf )
												{
													free( mt->data.buf );
												}
											free( mt );
										}
								}
							else
								{
									setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
									sendHeader( c_fd, &msg.hdr );

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );

									STOP = true;
								}
// printf( "OUT GP\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case GETPREVGROUPMSGS_OP :///< SEND GROUP CHAT HISTORY
						{
							me = connecting( users, msg.hdr.sender );
							node_t *tmp = me->mygroup->head;
							group_chat_t *g = NULL;

							pthread_mutex_lock( &me->mygroup->queue_lock );
								while( tmp != NULL )
									{
										g = tmp->ptr;

										if( me != NULL && g != NULL )
											{
												buff = ( char * )calloc( 1, sizeof( size_t ) );
												memcpy( buff, &g->messages->queue_len, sizeof( int ) );

												setHeader( &msg.hdr, OP_OK, "ChattyServer" );
												setData( &msg.data, me->nickname, buff, sizeof( int ) );

												sendHeader( c_fd, &msg.hdr );
												sendData( c_fd, &msg.data );

												if( msg.data.buf )
													{
														free( msg.data.buf );
													}

												pthread_mutex_lock( &g->messages->queue_lock );
													node_t *t = g->messages->head;
													message_t *mt = NULL;
													while( t != NULL )
														{
															mt = t->ptr;
															sendHeader( c_fd, &mt->hdr );
															sendData( c_fd, &mt->data );
															t = t->next;
														}
												pthread_mutex_unlock( &g->messages->queue_lock );
											}
										else
											{
												setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
												sendHeader( c_fd, &msg.hdr );

												pthread_mutex_lock( &chattyStats.statLock );
													chattyStats.nerrors += 1;
												pthread_mutex_unlock( &chattyStats.statLock );
											}
									tmp = tmp->next;
								}
						pthread_mutex_unlock( &me->mygroup->queue_lock );

						}break;
		//=============================================================================================//
		//=============================================================================================//
					case GETFILE_OP :///< SEND FILE TO THE RECEIVER USER
						{
// printf( "IN GF\n" );
							me = connecting( users, msg.hdr.sender );

							if( me != NULL )
								{
									struct stat sf;
									if( stat( msg.data.buf, &sf ) == -1 )
										{
											setHeader( &msg.hdr, OP_NO_SUCH_FILE, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
									else
										{
											message_data_t file;
											size_t len = sf.st_size;

											int fr;
											if( (fr = open( msg.data.buf, O_RDONLY, 0666 )) == -1 )
												{
													perror( "fopen" );

													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
													sendHeader( c_fd, &msg.hdr );

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}
											else
												{

													buff = ( char * )calloc( len, sizeof( char ) );
													if( read( fr, buff, len ) > 0 )
														{
															setHeader( &msg.hdr, OP_OK, "ChattyServer" );
															setData( &file, "", buff, len );

															sendHeader( c_fd, &msg.hdr );
															sendData( c_fd, &file );

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nfiledelivered += 1;
																if( chattyStats.nfilenotdelivered > 0 )
																	{
																		chattyStats.nfilenotdelivered -= 1;
																	}
															pthread_mutex_unlock( &chattyStats.statLock );

															close( fr );
														}
													else
														{
															setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
															sendHeader( c_fd, &msg.hdr );

															close( fr );
														}
												}
										}
								}
							else
								{
									setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
									sendHeader( c_fd, &msg.hdr );

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );
								}

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
// printf( "OUT GF\n\n" );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					default :
						{
							setHeader( &msg.hdr, OP_UNKNOWN, "ChattyServer" );
							sendHeader( c_fd, &msg.hdr );

							pthread_mutex_lock( &chattyStats.statLock );
								chattyStats.nerrors += 1;
							pthread_mutex_unlock( &chattyStats.statLock );

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
						}break;

		//=============================================================================================//
				}//END SWITCH
		}//END ELSE READMSG()


	if( STOP == true )
		{
			pthread_mutex_lock( &main_l );
				if( me != NULL )
					{
						pthread_mutex_lock( &users->ht_lock );
							me->fd_online = -1;
							me->alrdy_read = 0;
						pthread_mutex_unlock( &users->ht_lock );
					}
				con[i].fd = -1;
				con[i].revents = 0;
			pthread_mutex_unlock( &main_l );
			close( c_fd );
		}
	else
		{
			if( me != NULL )
				{
					pthread_mutex_lock( &users->ht_lock );
						con[i].fd = me->fd_online;
						con[i].revents = 0;
					pthread_mutex_unlock( &users->ht_lock );
				}
			else
				{
					pthread_mutex_lock( &main_l );
						con[i].fd = -1;
						con[i].revents = 0;
					pthread_mutex_unlock( &main_l );
					close( c_fd );
				}
		}

	free( args );
}



/**
 * @function				sig_handler
 * @brief			Function to manage the received signals
 * @param	sig		Signal code
 */
void sig_handler( int sig )
{
	switch( sig )
		{
			case SIGPIPE:
				{
					signal( sig, SIG_IGN );
					return;
				}break;

			case SIGINT:
			case SIGQUIT:
			case SIGTERM:
				{
					signal( sig, SIG_IGN );
					keepRUN = 0;
					return;
				}break;

			case SIGUSR1:
				{
					// pthread_mutex_lock( &main_l );
						printLOG = 1;
					// pthread_mutex_unlock( &main_l );
					return;
				}break;

			default:
				{
					write( 1, "Unmanaged Signal\n", strlen( "Unmanaged Signal\n" ) );
					return;
				}
		}
}



/******************************************************************************
                                    MAIN
******************************************************************************/
int main( int argc, char *argv[] )
{
    // >> VARIABLES DECLARATION << //
	struct sigaction 		s;					//signal to manage signals
	struct sockaddr_un 		server; 			//address server socket
	struct s_conf 			myconf; 			//server configuration fields
	struct pollfd 			*fds;				//struct to save the file descriptors to check them
	rq_arg 					*reqArg;			//connections handler fields
	hashtable_t 			*users = NULL; 		//users list
	threadpool_t 			*pool = NULL;		//thread pool of request handler worker
	int 					listener = 0;		//file descriptor of listener socket
	int 					new_client = 0;		//file descriptor of communication socket
	int						msec = 0;			//number to define the timeout of poll (in milliseconds)
	int						nfds = 0;			//number of file descriptors to check
	char 					opt = '\0';			//char to verify options of server
	char					*pathLogFile;		//address of server Log File


	// >> CONFIGURATION MANAGEMENT << //
	myconf = (struct s_conf) {
								.StatFileName = "",
								.DirName = "",
								.UnixPath = "",
								.MaxFileSize = 1024,
								.MaxConnections = 1,
								.ThreadsInPool = 1,
								.MaxMsgSize = 100,
								.MaxHistMsgs = 0
							};

	if (argc == 1)
		{
			usage( argv[0] );
			return -1;
		}
	while( (opt = getopt( argc, argv, "f" )) != -1 )
		{
			switch( opt )
				{
					case 'f' :
						{
							pars( argv[2], &myconf );
						}break;
					default:
						{
							usage( argv[0] );
							exit( EXIT_FAILURE );
						};
				}
		}



	// >> SERVER INIT << //
	configServer( &myconf );	//configures the server
	pathLogFile = myconf.StatFileName; //set log file path

	if( (users = initTable( _MAX_CONN )) == NULL )	//initalize users log
		{
			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
			return -1;
		}

	if( (pool = pool_creation( )) == NULL )	//initialize thread pool
		{
			for( int i = 0; i < _MAX_CONN; i++ )
				{
					if( users->users[i].user != NULL )
						{
							clear_chat( users->users[i].user->chats );
							free( users->users[i].user );
						}
					destroy_queue( users->users[i].collision );
				}

			free( users->users );
			free( users->groups );
			free( users );

			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
			return -1;
		}

	nfds = _MAX_CONN + 1; //set size of poll (plus 1 because there is the listener fd)

	if( myconf.ThreadsInPool <= 0 )
		{
			fprintf( stderr, "No defined threads!\n" );
			exit( EXIT_FAILURE );
		}
	if( myconf.MaxConnections <= 0 )
		{
			fprintf( stderr, "Not able to receive connections\n" );
		}


	// >> ALLOCATION ARRAY OF FDs << //
	if( (fds = ( struct pollfd * )malloc( nfds * sizeof( struct pollfd ) )) == NULL )
		{
			threadpool_destroy( pool, false );
			for( int i = 0; i < _MAX_CONN; i++ )
				{
					if( users->users[i].user != NULL )
						{
							clear_chat( users->users[i].user->chats );
							free( users->users[i].user );
						}
					if( users->users[i].collision != NULL )
						{
							node_t *n = users->users[i].collision->head;
							while( n != NULL )
								{
									user_t *usr = pull( users->users[i].collision );
									if( usr != NULL )
										{
											clear_chat( usr->chats );
											destroy_queue( usr->mygroup );
										}
									else
										{
											break;
										}
									free( usr );
								}
							destroy_queue( users->users[i].collision );
						}
				}
			free( users->users );
			free( users->groups );
			free( users );

			perror( "malloc:" );
			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
			exit( -1 );
		}



   // >> SET SIGNAL HANDLER << //
	memset( &s, 0, sizeof( s ) );

	s.sa_handler = &sig_handler;
	sigfillset(&s.sa_mask);

	//CATCH SIGPIPE
	if( sigaction(SIGPIPE, &s, NULL) == -1 )
		{
			perror( "sigaction SIGPIPE" );

			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
		}
	//CATCH SIGINT
	if( sigaction(SIGINT, &s, NULL) == -1 )
		{
			perror( "sigaction SIGINT" );

			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
		}
	//CATCH SIGTERM
	if( sigaction(SIGTERM, &s, NULL) == -1 )
		{
			perror( "sigaction SIGTERM" );

			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
		}
	//CATCH SIGQUIT
	if( sigaction(SIGQUIT, &s, NULL) == -1 )
		{
			perror( "sigaction SIGQUIT" );

			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
		}
	//CATCH SIGUSR1
	if( sigaction(SIGUSR1, &s, NULL) == -1 )
		{
			perror( "sigaction SIGUSR1" );
			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
		}

	unlink( SOCKNAME ); //delete name of socket from filesystem



    // >> SET SOCKET ADDRESS DOMAIN << //
	strncpy( server.sun_path, SOCKNAME, UNIX_PATH_MAX );
	server.sun_family = AF_UNIX;



	// >> BIND SOCKET NAME << //
	listener = socket( AF_UNIX, SOCK_STREAM, 0 );

 REOPEN_LISTENER: //label goto
	bind( listener, (struct sockaddr *)&server, sizeof(server) );



	// >> ZEROING ARRAY OF FDs << //
	for( int tfd = 0; tfd < nfds; tfd++ )
		{
			fds[tfd].fd = -1;
			fds[tfd].events = 0;
			fds[tfd].revents = 0x000;
		}



	// >> START LISTEN SOCKET << //
	listen( listener, _MAX_CONN );
	//SET LISTENER IN POLL STRUCTURE
	fds[nfds-1].fd = listener;
	fds[nfds-1].events = POLLIN;
	fds[nfds-1].revents = 0x000;

	msec = 0; //time for poll timeout



	// >> START MAIN (INFINITE) LOOP << //
	while( true )
	{				
		//SIGUSR1 PRINTER
		if( printLOG == 1 )
			{
				if( (pathLogFile == NULL) || (strcmp( pathLogFile, "" ) == 0) || (strcmp( pathLogFile, " " ) == 0) )
					{
						pthread_mutex_lock( &main_l );
							printLOG = 0;
						pthread_mutex_unlock( &main_l );
						continue;
					}
				print_statistic( users, pathLogFile );

				pthread_mutex_lock( &main_l );
					printLOG = 0;
				pthread_mutex_unlock( &main_l );

				continue;
			}


		//SET POLL STRUCTURES
		if( poll( fds, nfds, msec ) == -1 && keepRUN == 1 && printLOG == 0 )
			{
				pthread_mutex_lock( &chattyStats.statLock );
					chattyStats.nerrors += 1;
				pthread_mutex_unlock( &chattyStats.statLock );

				perror( "poll" );

				pthread_mutex_lock( &main_l );
					keepRUN = 0;
				pthread_mutex_unlock( &main_l );

			}


		//LOOP TO MONITOR ALL SOCKETS
		for( int tfd = 0; tfd < nfds; tfd++ )
			{
				//SET 'revent' OF LISTER TO SHUTDOWN SERVER
				if( keepRUN != 1 )
					{
						fds[nfds-1].revents = 0x011;
					}

				//SOCKET CLOSED
				if( fds[tfd].revents == 0x011 || fds[tfd].revents == 0x010 )
					{
						//SOCKET CLOSED IS THE LISTENER
						if( fds[tfd].fd == listener )
							{
								char r = '\0';
								if( keepRUN == 1 ) //socket closed by an error
								{
									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );

									fprintf( stderr, "\n'listen': Listener Socket was unexpectedly closed.\n" );
									fprintf( stdout, "Do you want to restart the socket without losing data? [Y|N]\n" );
									scanf( "%c", &r );
								}
								else //socket closed by a sig
								{
									r = 'N';
								}
								//SHUTDOWN THE SERVER
								if( r == 'N' || r == 'n' )
									{
										printf( "\n\n Shutdown ChattServer\n\n" );
										printf( " ...................................................................................................\n");
										//DESTROY USERS LIST
										node_t *au = users->active_user->head;
										while( au != NULL )
											{
												if( au->ptr != NULL )
													{
														remove_node( users->active_user, au );
														au = users->active_user->head;
													}
												else
													{
														break;
													}
											}
										destroy_queue( users->active_user );

										for( int i = 0; i < _MAX_CONN; i++ )
											{
												printf( " ...................................................................................................\n" );
												if( users->users[i].user != NULL )
													{

														clear_chat( users->users[i].user->chats );
														node_t *ng = users->users[i].user->mygroup->head;
														while( ng->ptr != NULL )
															{
																remove_node( users->users[i].user->mygroup, ng );
																ng = users->users[i].user->mygroup->head;
															}
														destroy_queue( users->users[i].user->mygroup );
														free( users->users[i].user );
													}
												if( users->users[i].collision != NULL )
													{
														node_t *n = users->users[i].collision->head;
														while( n != NULL)
															{
																user_t *usr = pull( users->users[i].collision );
																if( usr != NULL )
																	{
																		clear_chat( usr->chats );
																		node_t *ng = usr->mygroup->head;
																		while( ng->ptr != NULL )
																			{
																				remove_node( usr->mygroup, ng );
																				ng = usr->mygroup->head;
																			}
																		destroy_queue( usr->mygroup );
																	}
																else
																	{
																		break;
																	}
																free( usr );
															}
														destroy_queue( users->users[i].collision );
													}
											}
										free( users->users );
										printf( " ...................................................................................................\n" );


										for( int i = 0; i < _MAX_CONN; i++ )
											{
												printf( " ...................................................................................................\n" );
												if( users->groups[i].group != NULL )
													{
														clear_chat( users->groups[i].group->messages );
														if( users->groups[i].group->participants != NULL )
															{
																node_t *n = users->groups[i].group->participants->head;
																while( n->ptr != NULL )
																	{
																		remove_node( users->groups[i].group->participants, n );
																		n = users->groups[i].group->participants->head;
																	}
																destroy_queue( users->groups[i].group->participants );
															}
														free( users->groups[i].group );
													}
												if( users->groups[i].collision != NULL )
													{
														node_t *n = users->groups[i].collision->head;
														while( n != NULL )
															{
																group_chat_t *g = pull( users->groups[i].collision );
																if( g != NULL )
																	{
																		clear_chat( g->messages );
																		node_t *n = g->participants->head;
																		while( n->ptr != NULL )
																			{
																				remove_node( g->participants, g->participants->head );
																				n = users->groups[i].group->participants->head;
																			}
																		destroy_queue( g->participants );
																	}
																else
																	{
																		break;
																	}
																free( g );
															}
														destroy_queue( users->groups[i].collision );
													}
											}
										free( users->groups );

										free( users );
										printf( " ...................................................................................................\n" );

										free( fds );
										printf( " ...................................................................................................\n" );

										threadpool_destroy( pool, 2 );
										printf( " ...................................................................................................\n" );

										printf( "\n BYE\n\n" );
										exit( EXIT_SUCCESS );
									}
								//TRY TO RESTART THE LISTENER
								else 
									{
										fflush( stdout );
										fflush( stdin );
										fprintf( stderr, "Restart listener\n" );

										//CLOSE ALL OPEN SOCKETS
										for( int i = 0; i < _MAX_CONN; i++ )
											{
												//THE SOCKET IS IN USE
												if( i != -1 )
													{
														close( fds[i].fd );
													}
											}
										goto REOPEN_LISTENER;
									}
							}//end if (listener closed)
						//A CLIENT WAS ABORTED
						else
							{
// printf( "IN ABORT\n" );
								pthread_mutex_lock( &users->ht_lock );
									node_t *ntmp = users->active_user->head;
									user_t *utmp = NULL;

									//PUT USER OFFLINE
									int ack = 0;
									while( ntmp != NULL && !ack)
										{
											utmp = ntmp->ptr;
											if( utmp != NULL )
												{
													if( utmp->fd_online == fds[tfd].fd )
														{
															utmp->fd_online = -1;
															utmp->alrdy_read = 0;

															node_t *tmp = ntmp;
															ntmp = ntmp->next;
															ack = remove_node( users->active_user, tmp );
															// printf( "\tnode del\n" );
															break;

														}
												}
												// printf( "\tloop\n" );
											ntmp = ntmp->next;
										}
									fds[tfd].fd = -1;
								pthread_mutex_unlock( &users->ht_lock );
// printf( "OUT ABORT\n\n" );
							}

						if( fds[tfd].events != 0 )
							{
								continue;
							}
						else
						{
							close( fds[tfd].fd );
							fds[tfd].fd = -1;
							fds[tfd].events = 0;
						}
						continue;
					}//end if (socket closed)


				//NO EVENTS FOR TFD FD
				if( fds[tfd].revents == 0x000 )
					{
						continue;
					}


				//NEW CLIENT TRIES TO CONNECT
				if( fds[tfd].fd == listener )
					{
						//SET NON-BLOCKING LISTENER SOCKET
						int flags = fcntl( listener, F_GETFL, 0 );
						fcntl( listener, F_SETFL, flags | O_NONBLOCK );

						new_client = accept( listener, NULL, NULL );

						//THE CONNECTION FROM THE NEW CLIENT HAS NOT BEEN ACCEPTED
						if( new_client == -1 )
						{
							pthread_mutex_lock( &chattyStats.statLock );
								chattyStats.nerrors += 1;
							pthread_mutex_unlock( &chattyStats.statLock );

							perror( "accept" );
							continue;
						}

						//THE CONNECTION FROM THE NEW CLIENT HAS BEEN ACCEPTED
						else
							{
								//FIND FREE FD FIELD FOR A NEW CONNECTION
								int i = 0;
								while( (fds[i].fd != -1) && (i < _MAX_CONN) )
									{
										i++;
									}
								
								//FD IS FREE AND CAN ACCEPT A CONNECTION
								// pthread_mutex_lock( &main_l );
								if( fds[i].fd == -1 )
									{
										fds[i].fd = new_client;
										fds[i].events = POLLIN;

										int flags = fcntl( fds[i].fd, F_GETFL, 0 );
										if( fcntl( fds[i].fd, F_SETFL, flags | O_NONBLOCK ) == -1 )
											{
												pthread_mutex_unlock( &main_l );
												perror( "fcntl" );
												continue;
											}
									}
								//ALL FD ARE ACTIVE
								else
									{
										if( i >= _MAX_CONN )
											{
												message_t errmsg;
												setHeader( &errmsg.hdr, OP_TOO_MANY_CONN, "ChattyServer" );
												sendHeader( new_client, &errmsg.hdr );
												close( new_client );
											}
										// pthread_mutex_unlock( &main_l );
										continue;
									}
								// pthread_mutex_unlock( &main_l );
							}
					}//end if (client tries to connect)
				//A CLIENT SENT A REQUEST
				else
					{
						reqArg = (rq_arg *)malloc( sizeof( rq_arg ) );
						pthread_mutex_lock( &main_l );
							
							//SET ARGS TO PASS AT REQUESTS HANDLER
							reqArg->con = fds;
							reqArg->i_fd = tfd;
							reqArg->pool = pool;
							reqArg->users = users;
							reqArg->fd = fds[tfd].fd;

							fds[tfd].fd = -2;
							fds[tfd].revents = 0;

							threadpool_add( pool, &requests_handler, reqArg );
						pthread_mutex_unlock( &main_l );

					}
			}//close for() of poll
	}//close while(true)

	return 0;
}
