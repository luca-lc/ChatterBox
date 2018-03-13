/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */


/******************************************************************************
                                   HEADER
******************************************************************************/
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
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


pthread_mutex_t main_l = PTHREAD_MUTEX_INITIALIZER;

static volatile int keepRUN = 1;
static volatile int printLOG = 0;

/******************************************************************************
							STRUCT AND TYPEDEF
******************************************************************************/
/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0, PTHREAD_MUTEX_INITIALIZER }; //#mutex,#registered user,#client online,#delivered mes,#to be delivered message,#delivered files, #to be delivered files, #errors messages



/**
 *
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
 * 
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
 *
 */
static void usage(const char *progname)
{
    fprintf( stderr, "Il server va lanciato con il seguente comando:\n" );
    fprintf( stderr, "  %s -f <conffile>\n", progname );
}



/**
 *
 */
void update_st( hashtable_t *users )
{
	chattyStats.nonline = users->active_user->queue_len;
	chattyStats.nusers = users->reg_users;
}



/**
 *
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
			update_st( usr );
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
 *
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
 *
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
 *
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
 *
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
 *
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
											while( recvr->chats->queue_len > _MAX_HIST )
												{
													message_t *rm = pull( recvr->chats );
													free( rm->data.buf );
													free( rm );
												}
											if( sendHeader(	recvr->fd_online, &new_m->hdr ) > 0 )
												{
													if( sendData( recvr->fd_online, &new_m->data ) > 0 )
														{
															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.ndelivered += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
													else
														{
															err -= 1;

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nnotdelivered += 1;
																chattyStats.nerrors += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
												}
											else
												{
													err -= 1;

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nnotdelivered += 1;
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}
										}
									else
										{
											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nnotdelivered += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}

								}
							else
								{
									if( recvr == NULL )
										err -= 1000;
								}
						}
			// pthread_mutex_lock( &users->users[i].collision->queue_lock );
					if( users->users[i].collision != NULL )
						{

							node_t *tmp = users->users[i].collision->head;
							user_t *us = NULL;
							while( tmp != NULL )
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
															while( us->chats->queue_len > _MAX_HIST )
																{
																	message_t *rm = pull( us->chats );
																	free( rm->data.buf );
																	free( rm );
																}
															if( sendHeader( us->fd_online, &new_m->hdr ) > 0 )
																{
																	if( sendData( us->fd_online, &new_m->data ) > 0 )
																		{
																			pthread_mutex_lock( &chattyStats.statLock );
																				chattyStats.ndelivered += 1;
																			pthread_mutex_unlock( &chattyStats.statLock );
																		}
																	else
																		{
																			err -= 1;

																			pthread_mutex_lock( &chattyStats.statLock );
																				chattyStats.nnotdelivered += 1;
																				chattyStats.nerrors += 1;
																			pthread_mutex_unlock( &chattyStats.statLock );
																		}
																}
															else
																{
																	err -= 1;

																	pthread_mutex_lock( &chattyStats.statLock );
																		chattyStats.nnotdelivered += 1;
																		chattyStats.nerrors += 1;
																	pthread_mutex_unlock( &chattyStats.statLock );
																}
														}
													else
														{
															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nnotdelivered += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
												}
											else
												{
													err -= 1000;
												}
										}
									tmp = tmp->next;
								}
						}
				// pthread_mutex_unlock( &users->users[i].collision->queue_lock );
				}
		}
	if( new != NULL )
		{
			if( new->data.buf != NULL )
				{
					free( new->data.buf );
				}
			free( new );
		}
	return err;
}



/**
 *
 */
void clear_chat( queue_t *chat )
{
	message_t *tmsg;
	while( chat != NULL && chat->head != NULL )
		{
			tmsg = pull( chat );

			if( tmsg != NULL )
				{
					if( tmsg->data.buf != NULL )
						{
							free( tmsg->data.buf );
						}
				}
			else
				{
					pthread_mutex_lock( &chat->queue_lock );
						free( chat->head );
					pthread_mutex_unlock( &chat->queue_lock );
					break;
				}
		}
	
	free( chat );	
}



/**
 *
 */
char *rename_f( char *old )
{
    struct stat 	fs;
    char 			*new = NULL;
    int 			dirlen = strlen( Dir );
    int 			oldlen = strlen( old );
    int 			st_name = oldlen;
    int 			e = 0;
	int				slash = 0;

    DIR *chDir = opendir( Dir );
    if( !chDir )
        {
            mkdir( Dir, 0775 );
        }
    closedir( chDir );

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
                    new = (char *)realloc( new, (dirlen+slash+sn+se+lennnew+1) * sizeof( new ) );
                    
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
            name[sn] = '\0';
            strncpy( name, old+st_name, sn );
            name[sn] = '\0';

            new = ( char * )malloc( (dirlen+slash+sn) * sizeof( char ) );
			
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
                    new = (char *)realloc( new, (dirlen+slash+sn+lennnew+1) * sizeof( new ) );
                    
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
	printf( "\n<< %s >>\n", new );
	//    sleep( 5 );
    return new;
}


/**
 *
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
			int ack = 0;
			node_t *rm = users->active_user->head;
			user_t *u_rm = NULL;

			while( rm != NULL && !ack )
				{
					u_rm = rm->ptr;
					if( u_rm != NULL )
						{
							if( u_rm->fd_online == c_fd )
								{
									u_rm->fd_online = -1;
									ack = remove_node( users->active_user, rm );
									con[i].fd = -1;
								}
						}
					rm = rm->next;
				}
			if( ack == 0 )
				{
					setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
					pthread_mutex_lock( &chattyStats.statLock );
						chattyStats.nerrors += 1;
					pthread_mutex_unlock( &chattyStats.statLock );
					
					con[i].fd = c_fd;
					sendHeader( c_fd, &msg.hdr );
				}
		}
	else
		{
			// MANAGE REQUEST DEPENDING ON THE 'OP'
			switch( msg.hdr.op )
				{
		//===================================================================//
					case REGISTER_OP :
						{
printf( "REG << %s >>\n", msg.hdr.sender );
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
printf( "REGout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case CONNECT_OP :
						{
							me = connecting( users, msg.hdr.sender );
printf( "CONN << %s >>\n", me->nickname );
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
printf( "CONNout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case USRLIST_OP :
						{
							me = connecting( users, msg.hdr.sender );
printf( "ULST << %s >>\n", me->nickname );
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
printf( "ULSTout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case DISCONNECT_OP :
						{
							me = connecting( users, msg.hdr.sender );
printf( "DISC << %s >>\n", me->nickname );
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
											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}
							else
								{
									setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );

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
							sendHeader( c_fd, &msg.hdr );
printf( "DISCout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case UNREGISTER_OP :
						{
							me = connecting( users, msg.hdr.sender );
printf( "UNREG << %s >>\n", me->nickname );
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
												}
											else
												{
													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );

													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
												}

											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}
							else
								{
									setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );
								}

							sendHeader( c_fd, &msg.hdr );

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
printf( "UNREGout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case CREATEGROUP_OP :
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
													push( g->participants, me );
													push( me->mygroup, g );

													setHeader( &msg.hdr, OP_OK, "ChattyServer" );
												}
											else
												{
													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );

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

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );

											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_NICK_AVAILABLE, "ChattyServer" );

											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}

							sendHeader( c_fd, &msg.hdr );

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
					case ADDGROUP_OP :
						{
							char 			*gname = msg.data.hdr.receiver;
							group_chat_t 	*g = NULL;

							me = connecting( users, msg.hdr.sender );
							
							pthread_mutex_lock( &users->ht_lock );
								g = searchGroup( users, gname );
							pthread_mutex_unlock( &users->ht_lock );

							if( me != NULL && g != NULL )
								{
									if( push( g->participants, me ) >= 0 && push( me->mygroup, g ) >= 0 )
										{
											setHeader( &msg.hdr, OP_OK, "ChattyServer" );
										}
									else
										{
											setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
											
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
											
											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
										}
									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );
								}
							sendHeader( c_fd, &msg.hdr );

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
					case DELGROUP_OP :
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
												}
											else
												{
													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
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
															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nerrors += 1;
															pthread_mutex_unlock( &chattyStats.statLock );
														}
												}
											else
												{
													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
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
											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
											STOP = true;
										}
									else
										{
											setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
								}
							sendHeader( c_fd, &msg.hdr );

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
					case POSTTXT_OP :
						{
							pthread_mutex_lock( &users->ht_lock );
								group_chat_t *g = searchGroup( users, msg.data.hdr.receiver );
							pthread_mutex_unlock( &users->ht_lock );

							user_t *recvr = connecting( users, msg.data.hdr.receiver );
							me = connecting( users, msg.hdr.sender );
printf( "TXT << %s >>\n", me->nickname );
							if( me != NULL && (recvr != NULL || g != NULL ) )
								{

									if( msg.data.hdr.len > _MSGSIZE )
										{
											setHeader( &msg.hdr, OP_MSG_TOOLONG, "ChattyServer" );

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
																			new_m->data.buf = ( char * )malloc( new->data.hdr.len * sizeof( char ) );
																			copyMSG( new_m, new );

																			user_t *u = tmp_s->ptr;
																			if( push( u->chats, new_m ) == 0 )
																				{
																					if( u->fd_online != -1 )
																						{
																							sendHeader(	u->fd_online, &new_m->hdr );
																							sendData( u->fd_online, &new_m->data );
																							
																							pthread_mutex_lock( &chattyStats.statLock );
																								chattyStats.ndelivered += 1;
																							pthread_mutex_unlock( &chattyStats.statLock );
																						}
																					else
																						{
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
																	pthread_mutex_lock( &chattyStats.statLock );
																		chattyStats.nnotdelivered += 1;
																		chattyStats.nerrors += 1;
																	pthread_mutex_unlock( &chattyStats.statLock );

																	STOP = true;
																	
																	sendHeader( c_fd, &msg.hdr );
																}
														}
													else
														{
															setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );

															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nnotdelivered += 1;
																chattyStats.nerrors += 1;
															pthread_mutex_unlock( &chattyStats.statLock );

															sendHeader( c_fd, &msg.hdr );
														}
												}
											else
												{
													setHeader( &new->hdr, TXT_MESSAGE, me->nickname );

													if( push( recvr->chats, new ) == 0 )
														{
															setHeader( &msg.hdr, OP_OK, "ChattyServer" );
															if( recvr->fd_online != -1 )
																{
																	while( recvr->chats->queue_len >= _MAX_HIST )
																		{
																			message_t *rm = pull( recvr->chats );
																			free( rm->data.buf );
																			free( rm );
																		}
																	if( sendHeader( recvr->fd_online, &new->hdr ) > 0 )
																		{
																			if( sendData( recvr->fd_online, &new->data ) > 0 )
																				{
																					setHeader( &msg.hdr, OP_OK, "ChattyServer" );
																					sendHeader( c_fd, &msg.hdr );
																					pthread_mutex_lock( &chattyStats.statLock );
																						chattyStats.ndelivered += 1;
																					pthread_mutex_unlock( &chattyStats.statLock );
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
															else
																{
																	sendHeader( c_fd, &msg.hdr );

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
											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nnotdelivered += 1;
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
									else
										{
											setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
											pthread_mutex_lock( &chattyStats.statLock );
												chattyStats.nnotdelivered += 1;
												chattyStats.nerrors += 1;
											pthread_mutex_unlock( &chattyStats.statLock );
										}
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
printf( "TXTout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case POSTTXTALL_OP :
						{
							me = connecting( users, msg.hdr.sender );
printf( "ALL << %s >>\n", me->nickname );
							if( me != NULL )
								{
									if( msg.data.hdr.len > _MSGSIZE )
										{
											setHeader( &msg.hdr, OP_MSG_TOOLONG, "ChattyServer" );

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

											setHeader( &new->hdr, TXT_MESSAGE, me->nickname );
											int ack;
											pthread_mutex_lock( &users->ht_lock );
												ack = sendtoall( new, users );
											pthread_mutex_unlock( &users->ht_lock );
											if( ack != 0 )
												{
													if( ack % 1000 == 0 )
														{
															setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
														}
													else
														{
															setHeader( &msg.hdr, MSG_NDELIV_SOMEUSERS, "ChattyServer" );
														}
												}
										}
								}
							else
								{
									STOP = true;
									setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
								}
							sendHeader( c_fd, &msg.hdr );

							if( buff )
								{
									free( buff );
								}
							if( msg.data.buf )
								{
									free( msg.data.buf );
								}
printf( "ALLout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case POSTFILE_OP :
						{
							user_t *recvr = connecting( users, msg.data.hdr.receiver );
							me = connecting( users, msg.hdr.sender );
                            FILE *_f_tmp;
printf( "FILE << %s >>\n", me->nickname );
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
											pthread_mutex_lock( &main_l );
												message_data_t file;
												message_t *new = ( message_t * )malloc( sizeof( message_t ) );
												
                                                char *newf = rename_f( msg.data.buf );
												if ( (_f_tmp = open( newf,  O_CREAT|O_WRONLY|O_TRUNC, 0666 )) == -1 )
													{
														perror( "open" );
													}	
											 	
                                            new->data.buf = (char *)malloc( strlen( newf ) * sizeof( char ) );
											copyMSG( new, &msg );

											// memset( &new->data.buf[0], 0, strlen( msg.data.buf ) );
											strcpy( new->data.buf, newf );
											new->data.hdr.len = strlen( newf );
											free( newf );
											pthread_mutex_unlock( &main_l );

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
																			close( _f_tmp );
																		}
																	else
																		{
																			write( _f_tmp, file.buf, file.hdr.len );
																			close( _f_tmp );
                  
																			if( push( g->messages, new ) == 0 )
																				{
                                                                                    setHeader( &msg.hdr, OP_OK, "ChattyServer" );
                                                                                    sendHeader( c_fd, &msg.hdr );
																					
                                                                                    pthread_mutex_lock( &g->participants->queue_lock );																			
																						node_t *tmp_s = g->participants->head;
																						while( tmp_s != NULL )
																							{
																								message_t *new_m = (message_t *)malloc( sizeof( message_t ) );
																								new_m->data.buf = (char *)malloc( new->data.hdr.len * sizeof( char ) );
																								copyMSG( new_m, new );

																								user_t *u = tmp_s->ptr;
																								if( push( u->chats, new_m ) == 0 )
																									{
																										if( u->fd_online != -1 )
																											{
																												sendHeader(	u->fd_online, &new_m->hdr );
																												sendData( u->fd_online, &new_m->data );
																												
																												pthread_mutex_lock( &chattyStats.statLock );
																													chattyStats.nfiledelivered += 1;
																												pthread_mutex_unlock( &chattyStats.statLock );
																											}
																									}
																								tmp_s = tmp_s->next;
																							}
																					pthread_mutex_unlock( &g->participants->queue_lock );
																					
																					pthread_mutex_lock( &chattyStats.statLock );
																						chattyStats.ndelivered += 1;
																					pthread_mutex_unlock( &chattyStats.statLock );
																				}
																		}

																	if( file.buf )
																		{
																			free( file.buf );
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
													setHeader( &new->hdr, FILE_MESSAGE, me->nickname );

													if( readData( c_fd, &file ) > 0 )
														{
															if( file.hdr.len > _FILESIZE )
																{
																	setHeader( &msg.hdr, OP_MSG_TOOLONG, "ChattyServer" );
																	
																	pthread_mutex_lock( &chattyStats.statLock );
																		chattyStats.nerrors += 1;
																	pthread_mutex_unlock( &chattyStats.statLock );
																	close( _f_tmp );
																	remove( new->data.buf );
																}
															else
																{
                                                                    write( _f_tmp, file.buf, file.hdr.len );
																	close( _f_tmp );


																	if( push( recvr->chats, new ) == 0 )
																		{
																			pthread_mutex_lock( &chattyStats.statLock );
																				chattyStats.nfilenotdelivered += 1;
																			pthread_mutex_unlock( &chattyStats.statLock );
																			
																			setHeader( &msg.hdr, OP_OK, "ChattyServer" );
                                                                            sendHeader( c_fd, &msg.hdr );

																			if( recvr->fd_online != -1 )
																				{																		
																					while( recvr->chats->queue_len >= _MAX_HIST )
																						{
																							message_t *rm = pull( recvr->chats );
																							free( rm->data.buf );
																							free( rm );
																						}
																					if( sendHeader( recvr->fd_online, &new->hdr ) > 0 )
																						{
																							if( sendData( recvr->fd_online, &new->data ) > 0 )
																								{
                                                                                                    pthread_mutex_lock( &chattyStats.statLock );
																										chattyStats.nfiledelivered += 1;
																										chattyStats.nfilenotdelivered -= 1;	
																									pthread_mutex_unlock( &chattyStats.statLock );
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
																				}
																		}
																}
															if( file.buf )
																{
																	free( file.buf );
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

															close( _f_tmp );
															remove( new->data.buf );
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
printf( "FILEout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case GETPREVMSGS_OP : 
						{
							me = connecting( users, msg.hdr.sender );
printf( "MSGPREV << %s >>\n", me->nickname );
							if( me != NULL )
								{
										buff = ( char * )malloc( sizeof( size_t ) );
										// memset( &buff[0], 0, sizeof( int ) );
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
														pthread_mutex_lock( &chattyStats.statLock );
															chattyStats.ndelivered += 1;
														pthread_mutex_unlock( &chattyStats.statLock );
													}
											}
									pthread_mutex_unlock( &me->chats->queue_lock );

									while( me->chats->queue_len > _MAX_HIST )
									{
										mt = pull( me->chats );
										if( mt->data.buf != NULL )
											{
												free( mt->data.buf );
											}
										free( mt );
									}
								}
							else
								{
									setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );
									sendHeader( c_fd, &msg.hdr );

									STOP = true;
								}
printf( "MSGPREVout << %s >>\n", me->nickname );
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case GETPREVGROUPMSGS_OP :
						{
							me = connecting( users, msg.hdr.sender );
							node_t *tmp = me->mygroup->head;
							
							pthread_mutex_lock( &users->ht_lock );
								group_chat_t *g = tmp->ptr;
								while( tmp != NULL && strcmp( g->chat_title, msg.data.hdr.receiver ) != 0 )
									{
										tmp = tmp->next;
										g = tmp->ptr;
									}
							pthread_mutex_unlock( &users->ht_lock );

							if( me != NULL && tmp != NULL && g != NULL )
								{
									buff = ( char * )malloc( sizeof( int ) * sizeof( char ) );
									memcpy( buff, &g->messages->queue_len, sizeof( int ) );

									setHeader( &msg.hdr, OP_OK, "ChattyServer" );
									setData( &msg.data, me->nickname, buff, sizeof( int ) );

									sendHeader( c_fd, &msg.hdr );
									sendData( c_fd, &msg.data );
									
									pthread_mutex_lock( &g->messages->queue_lock );
										node_t *t = g->messages->head;
										message_t *mt = NULL;
										while( t != NULL && t->ptr != NULL )
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

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );

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
						}break;
		//=============================================================================================//
		//=============================================================================================//
					case GETFILE_OP :
						{
							me = connecting( users, msg.hdr.sender );
printf( "MSGFILE << %s >>\n", me->nickname );
							if( me != NULL )
								{
									struct stat sf;
									if( stat( msg.data.buf, &sf ) == -1 )
										{
											setHeader( &msg.hdr, OP_NO_SUCH_FILE, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );
										}
									else
										{
											setHeader( &msg.hdr, OP_OK, "ChattyServer" );
											sendHeader( c_fd, &msg.hdr );

											message_data_t file;
											size_t len = sf.st_size;

											int *fr;
											if( (fr =open( msg.data.buf, O_RDONLY, 0666 )) == NULL )
												{
													perror( "fopen" );

													setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
													pthread_mutex_lock( &chattyStats.statLock );
														chattyStats.nerrors += 1;
													pthread_mutex_unlock( &chattyStats.statLock );
													sendHeader( c_fd, &msg.hdr );
												}
											else
												{
													buff = ( char * )malloc( len * sizeof( char ) );
													if( read( fr, buff, len ) > 0 )
														{
															setData( &file, "", buff, len );
															sendData( c_fd, &file );
															pthread_mutex_lock( &chattyStats.statLock );
																chattyStats.nfiledelivered += 1;
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

									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );

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
printf( "MSGFILEout << %s >>\n", me->nickname );
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
 *
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
					pthread_mutex_lock( &main_l );
						printLOG = 1;
					pthread_mutex_unlock( &main_l );
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
										.MaxHistMsgs = 1
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
	configServer( &myconf );
	pathLogFile = myconf.StatFileName;
	if( (users = initTable( _MAX_CONN )) == NULL )
		{
			pthread_mutex_lock( &chattyStats.statLock );
				chattyStats.nerrors += 1;
			pthread_mutex_unlock( &chattyStats.statLock );
			return -1;
		}
	if( (pool = pool_creation( )) == NULL )
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
	nfds = _MAX_CONN + 1; //1 because there is the listener fd


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

	msec = 2; //time for poll timeout

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

				//SOCKET CLOSED WITH ERROR
				if( fds[tfd].revents == 0x011 || fds[tfd].revents == 0x010 )
					{
						//SOCKET CLOSED IS LISTENER
						if( fds[tfd].fd == listener )
							{
								char r = '\0';
								if( keepRUN == 1 ) //socket closed with error
								{
									pthread_mutex_lock( &chattyStats.statLock );
										chattyStats.nerrors += 1;
									pthread_mutex_unlock( &chattyStats.statLock );

									fprintf( stderr, "\n'listen': Listener Socket was unexpectedly closed.\n" );
									fprintf( stdout, "Do you want to restart the socket without losing data? [Y|N]\n" );
									scanf( "%c", &r );
								}
								else //socket closed from SIGINT
								{
									r = 'N';
								}
								if( r == 'N' || r == 'n' ) //shutdown server
									{
										printf( "\n\n Shutdown service\n ..............................................\n " );
																				
										//DESTROY USERS LIST
										destroy_queue( users->active_user );
										
										sleep( 1 );

										for( int i = 0; i < _MAX_CONN; i++ )
											{
												printf( ".............................................." );
												if( users->users[i].user != NULL )
													{

														clear_chat( users->users[i].user->chats );
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
										printf( "\n ..............................................\n" );

										free( users );
										printf( " ..............................................\n" );

										free( fds );
										printf( " ..............................................\n" );

										sleep( 1 );

										//DESTROY THREADPOOL
										threadpool_destroy( pool, false );
										printf( " ..............................................\n" );

										printf( "\n BYE\n\n" );
										exit( EXIT_SUCCESS );
									}
								else //try to restart the listener socket
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
							}
						//CONNECTION WITH THE CLIENT WAS ABORTED
						else
							{
								pthread_mutex_lock( &users->ht_lock );
									node_t *ntmp = users->active_user->head;
									user_t *utmp = NULL;

									//PUT USER OFFLINE
									int ack = 0;
									while( ntmp != NULL && !ack )
										{
											utmp = ntmp->ptr;
											if( utmp != NULL )
												{
													if( utmp->fd_online == fds[tfd].fd )
														{
															utmp->fd_online = -1;
															ack = remove_node( users->active_user, ntmp );
														}
												}
											ntmp = ntmp->next;
										}
									fds[tfd].fd = -1;
								pthread_mutex_unlock( &users->ht_lock );
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
					}

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
								pthread_mutex_lock( &main_l );
								if( fds[i].fd == -1 )
									{
										fds[i].fd = new_client;
										fds[i].events = POLLIN;

										int flags = fcntl( fds[i].fd, F_GETFL, 0 );
										if( fcntl( fds[i].fd, F_SETFL, flags | O_NONBLOCK ) == -1 )
											{
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
										continue;
									}
								pthread_mutex_unlock( &main_l );
							}
					}
				//A CLIENT SENT A REQUEST
				else
					{
						reqArg = (rq_arg *)malloc( sizeof( rq_arg ) );
						//SET ARGS TO PASS AT REQUESTS HANDLER
						pthread_mutex_lock( &main_l );

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