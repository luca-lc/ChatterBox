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
#include <src/stats.h>
#include <src/queue.h>
#include <src/signup.h>
#include <src/ops.h>
#include <src/pool.h>
#include <time.h>
#include <ctype.h>
#include <src/connections.h>
#include <src/hashtable.h>
#include "src/chatty.h"
#include <src/config.h>

/* inserire gli altri include che servono */
/******************************************************************************
								DEFINES
******************************************************************************/
/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 }; //#registered user,#client online,#delivered mes,#to be delivered message,#delivered files, #to be delivered files, #errors messages

typedef struct arg_mng
{
	hashtable_t *users;
	int con_fd;
}arg_con;
/******************************************************************************
                                FUNCTIONS
******************************************************************************/



/**
 * TODO
 */
static void usage(const char *progname)
{
    fprintf( stderr, "Il server va lanciato con il seguente comando:\n" );
    fprintf( stderr, "  %s -f <conffile>\n", progname );
}



/**
 *
 */
void print_statistic( char *FileName )
{

	//check if 'FileName' exists
	struct stat st;
	if( stat( FileName, &st ) != -1 )
	{
		remove( FileName );
	}


	FILE *stats;
	int stat_o_time = 5;

REOPEN:
	stat_o_time = 5;
	while( (stats = fopen( FileName, "a+" )) == NULL && stat_o_time > 0 )
	{
		perror( "fopen()" );
		fprintf( stderr, "%s\n\n", FileName );
		fprintf( stderr, "Try again %d times\n",  stat_o_time );
		stat_o_time --;
	}
	if( stat_o_time == 0 )
	{
		fprintf( stderr, "Unable to open the file %s\n", FileName );
	}

	int r = 0;

	while( true && r == 0 )
	{
		r = printStats( stats );
		sleep( 30 );
	}

	close( stats );
	if( r != 0 )
	{
		fprintf( stderr, "Some problems to write the file, try to re-open %s\n", FileName );
goto REOPEN;
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
void mng_conn( void *args )
{

	//explode the arguments
	arg_con *mycon = (arg_con *)args;
	int c_fd = mycon->con_fd;
	hashtable_t *users = mycon->users;

	//set local variables
	message_t msg;
	user_t *me = NULL;
	char *buff = NULL;
	bool STOP = false;

	printf( "\n--------------------------------------\n" );
	printf( "Connection %d has been started", c_fd );
	printf( "\n--------------------------------------\n\n" );

	do
	{
		msg.hdr.op = -1;
		if( readMsg( c_fd, &msg ) <= 0)
		{
			struct sigaction s;
			memset( &s, 0, sizeof( s ) );
			s.sa_handler=SIG_IGN;

			if( sigaction( SIGPIPE, &s, NULL ) == -1 )
			{
				perror( "sigaction" );
			}

			msg.hdr.op = DISCONNECT_OP;
		}

		switch( msg.hdr.op )
		{

		/*-----------------------------------------*/

			case REGISTER_OP :
				{
					int ack_reg;
					ack_reg = checkin( users, msg.hdr.sender );

					if( ack_reg == true ) //if the user has been registered
					{
						me = connecting( users, msg.hdr.sender );
						if( me != NULL ) //if the user is logged on
						{
							char *con_me = (char *)malloc( MAX_NAME_LENGTH * sizeof( char ) );
							strcpy( con_me, me->nickname );
							push( users->active_user, con_me );

							//set user list to send
							sendUlist( users->active_user, &msg, me, buff ); //set the data in this function

							setHeader( &msg.hdr, OP_OK, "ChattyServer" );

							sendHeader( c_fd, &msg.hdr );
							sendData( c_fd, &msg.data );

							free( buff );
						}
						else //if the user is not logged on
						{
							setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
							sendHeader( c_fd, &msg.hdr );
							STOP = true;
						}
					}
					else //if the user has not been registered
					{
						if( ack_reg == false )
						{
							setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
						}
						else
						{
							setHeader( &msg.hdr, OP_NICK_ALREADY, "ChattyServer" );
						}

						sendHeader( c_fd, &msg.hdr );
						STOP = true;
					}
				}break;

	/*-----------------------------------------*/

			case CONNECT_OP :
				{
					me = connecting( users, msg.hdr.sender );
					if( me != NULL ) //if the user is logged on
					{
						char *con_me = (char *)malloc( MAX_NAME_LENGTH * sizeof( char ) );
						strcpy( con_me, me->nickname );

						if( users->active_user == NULL )
						{
							setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
							sendHeader( c_fd, &msg.hdr );
							STOP = true;
						}
						else
						{
							//check if already online
							node_t *tmp_n = users->active_user->head;
							user_t *tmp_u = NULL;
							int l_stop = 0;

							while( tmp_n != NULL && !l_stop )
							{
								tmp_u = tmp_n->ptr;
								if( tmp_u != NULL )
								{
									( strcmp((char *)tmp_u, me->nickname) == 0 ) ? ( l_stop = 1 ) : ( tmp_n = tmp_n->next );
								}
								else
								{
									tmp_n = NULL;
								}
							}

							if( !l_stop )//if not online
							{
								push( users->active_user, con_me );

								//set user list to send
								sendUlist( users->active_user, &msg, me, buff ); //set the data in this function
								setHeader( &msg.hdr, OP_OK, "ChattyServer" );

								sendHeader( c_fd, &msg.hdr );
								sendData( c_fd, &msg.data );

								free( buff );
							}
							else//if online
							{
								setHeader( &msg.hdr, OP_ALREADY_ONLINE, "ChattyServer" );
								sendHeader( c_fd, &msg.hdr );

								STOP = true;
							}
						}
					}
					else
					{
						setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
						sendHeader( c_fd, &msg.hdr );

						STOP = true;
					}
				}break;

	/*-----------------------------------------*/

			case USRLIST_OP :
				{
					//set user list to send
					sendUlist( users->active_user, &msg, me, buff ); //set the data in this function

					setHeader( &msg.hdr, OP_OK, "ChattyServer" );

					sendHeader( c_fd, &msg.hdr );
					sendData( c_fd, &msg.data );

					free( buff );

				}break;

	/*-----------------------------------------*/

			case DISCONNECT_OP :
				{
					node_t *rm = users->active_user->head;
					char *u_rm = rm->ptr;

					while( rm != NULL && strcmp(u_rm, me->nickname) != 0 )
					{
						rm = rm->next;
						u_rm = rm->ptr;
					}

					int ack = remove_node( users->active_user, rm );
					if( ack == 1 )
					{
						setHeader( &msg.hdr, OP_OK, "ChattyServer" );
						STOP = true;
					}
					else
					{
						setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
					}

					sendHeader( c_fd, &msg.hdr );
				}break;

	/*-----------------------------------------*/

			case UNREGISTER_OP :
				{
					node_t *rm_n = users->active_user->head;
					char *u_rm = rm_n->ptr;

					//put user offline
					while( rm_n != NULL && strcmp(u_rm, me->nickname) != 0 )
					{
						rm_n = rm_n->next;
						u_rm = rm_n->ptr;
					}

					int ack = remove_node( users->active_user, rm_n );
					if( ack == 1 )
					{
						int d_ack = delete( users, me->nickname );
						d_ack == true ? setHeader( &msg.hdr, OP_OK, "ChattyServer" ) : setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );

						STOP = true;
					}
					else
					{
						setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
					}

					sendHeader( c_fd, &msg.hdr );
				}break;

	/*-----------------------------------------*/

			case CREATEGROUP_OP :
				{
					char *gname = msg.data.hdr.receiver;
					node_t *node = users->groups->head;
					group_chat_t *group = NULL;
					int lstop = 0;

					while( node != NULL && !lstop )
					{
						group = node->ptr;
						if( group != NULL )
						{
							( strcmp(group->chat_title, gname) == 0 ) ? ( lstop = 1 ) : ( node = node->next );
						}
						else
						{
							node = NULL;
						}
					}

					if( !lstop && me != NULL )
					{
						group_chat_t *new_group = NULL;
						if( (new_group = (group_chat_t *)malloc( sizeof( group_chat_t ) )) == NULL )
						{
							setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
						}
						else
						{
							new_group->participants = initialQueue();
							new_group->messages = initialQueue();

							if( new_group->participants != NULL && new_group->messages != NULL )
							{
								char *tmp_u = (char *)malloc( MAX_NAME_LENGTH * sizeof( char ) );

								strcpy( new_group->chat_title, gname ); //set group name
								strcpy( tmp_u, me->nickname ); //copy user name

								push( new_group->participants, tmp_u ); //enter first user in the group

								push( users->groups, new_group );

								setHeader( &msg.hdr, OP_OK, "ChattyServer" );
							}
							else
							{
								free( new_group );
								setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
							}
						}
					}
					else
					{
						if( lstop )
						{
							setHeader( &msg.hdr, OP_NICK_ALREADY, "ChattyServer" );
						}
						else
						{
							setHeader( &msg.hdr, OP_NICK_UNKNOWN, "ChattyServer" );
							STOP = true;
						}
					}

					sendHeader( c_fd, &msg.hdr );
				}break;

	/*-----------------------------------------*/

			case ADDGROUP_OP :
				{
					char *gname = msg.data.hdr.receiver;
					node_t *gnode = users->groups->head;
					group_chat_t *group = NULL;
					int l_stop = 0;

					while( gnode != NULL && !l_stop )
					{
						group = gnode->ptr;

						if( group != NULL )
						{
							( strcmp( group->chat_title, gname ) == 0 ) ? ( l_stop = 1 ) : ( gnode = gnode->next );
						}
						else
						{
							gnode = NULL;
						}
					}

					if( l_stop )
					{
						node_t *unode = group->participants->head;
						user_t *usr = NULL;
						int lstop = 0;

						while( unode != NULL && !lstop )
						{
							usr = unode->ptr;
							if( usr != NULL )
							{
								( strcmp( usr, me->nickname ) == 0 ) ? ( lstop = 1 ) : ( unode = unode->next );
							}
							else
							{
								unode = NULL;
							}
						}
						if( !lstop )
						{
							char *new_p = NULL;
							if( (new_p = (char *)malloc( MAX_NAME_LENGTH * sizeof( char ) )) != NULL )
							{
								push( group->participants, new_p );
								setHeader( &msg.hdr, OP_OK, "ChattyServer" );
							}
							else
							{
								setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
							}
						}
						else
						{
							setHeader( &msg.hdr, OP_NICK_AVAILABLE, "ChattyServer" );
						}
					}
					else
					{
						setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
					}

					sendHeader( c_fd, &msg.hdr );
				}break;

	/*-----------------------------------------*/

			case DELGROUP_OP :
				{
					char *gname = msg.data.hdr.receiver;
					node_t *node = users->groups->head;

					group_chat_t *group = NULL;
					int lstop = 0;
					while( node != NULL && !lstop )
					{
						group = node->ptr;
						if( group != NULL )
						{
							( strcmp( group->chat_title, gname ) == 0 ) ? ( lstop = 1 ) : ( node = node->next );
						}
						else
						{
							node = NULL;
						}
					}

					if( lstop )
					{
						destroy_queue( group->messages );
						destroy_queue( group->participants );

						(remove_node( users->groups, node ) == 1 ) ? setHeader( &msg.hdr, OP_OK, "ChattyServer" ) : setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );;
					}
					else
					{
						setHeader( &msg.hdr, OP_GROUP_UNKNOWN, "ChattyServer" );
					}

					sendHeader( c_fd, &msg.hdr );

				}break;

	/*-----------------------------------------*/

			default :
				{
					printf("Unknown Operation\n");
				}break;

	/*-----------------------------------------*/
		}//END SWITCH

	}while( !STOP );





	printf( "\n--------------------------------------\n" );
	printf( "Connection %d has been stopped", c_fd );
	printf( "\n--------------------------------------\n\n" );

	close( c_fd );

}




/******************************************************************************
                                    MAIN
******************************************************************************/
int main(int argc, char *argv[])
{

    /** VARIABLES **/
    struct s_conf myconf;
    arg_con con;
    hashtable_t *users;


    //gestione esecuzione server
    if (argc == 1)
    {
        usage( argv[0] );
        return -1;
    }
    char opt;
    while( (opt = getopt( argc, argv, "f" )) != -1 )
    {
        switch( opt )
        {
            case 'f' :
            {
            	pars( argv[2], &myconf );
            }
                break;

            default:
            {
            	usage( argv[0] );
            };
        }
    }

    //start thread to print statics
    pthread_t stat_printer;
    pthread_create( &stat_printer, NULL, (void *)print_statistic, (void *)myconf.StatFileName );


    //init server
    configServer( &myconf );
    users = initTable( _MAX_CONN );
    threadpool_t *pool = pool_creation( );


    //TEST

    unlink(SOCKNAME);

    int s_fd, c_fd;


    struct sockaddr_un se;
    strncpy( se.sun_path, SOCKNAME, UNIX_PATH_MAX );
    se.sun_family = AF_UNIX;

    s_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    bind( s_fd, (struct sockaddr *)&se, sizeof(se) );
    listen( s_fd, _MAX_CONN );

    while( true )
    {
    	c_fd = accept( s_fd, NULL, 0 );
    	if( c_fd != -1 )
    	{
    		con.con_fd = c_fd;
    		con.users = users;
    		threadpool_add( pool, mng_conn, &con );
    	}
    	else
    	{
    		fprintf( stderr, "ERROR ESTABLISHING CONNECTION\n");
    		continue;
    	}

    }




//    hashtable_t *users = initTable( _MAX_CONN );

//    users->users[0].user = malloc( sizeof(user_t) );
//    users->users[0].user->key = 1995;
//    users->users[0].user->nickname = (char *)malloc( 10 *sizeof( char));
//    strcpy( users->users[0].user->nickname, "luca" );
//    printf( "%s - %d\n", users->users[0].user->nickname, users->users[0].user->key );
//    insert( users, "luca" );
////
//    insert( users, "lucia" );
//    insert( users, "roby" );
//    insert( users, "betta" );
//    insert( users, "luciano" );
//    insert( users, "licia" );

//
//    printf( "puppa\n" );
//
//
//	user_t *b = search( users, "betta" );
//	printf( "%s - %d\n", b->nickname, b->key );
//
//	b = search( users, "luca" );
//	printf( "%s - %d\n", b->nickname, b->key );
//
//	b = search( users, "roby" );
//	printf( "%s - %d\n", b->nickname, b->key );
//
//	b = search( users, "Fuffy" );
//	if( b != NULL )
//		printf( "%s - %d\n", b->nickname, b->key );
//	else
//		printf( "puppa" );
//
//	b = search( users, "lucia" );
//	printf( "%s - %d\n", b->nickname, b->key );
//
//	int c = removing( users, "luca" );
//	c = removing( users, "lucia" );
//	c = removing( users, "lucia" );

//	if( c == 1 )
//	{
//		b = search( users, "lucia" );
//		printf( "%s - %d\n", b->nickname, b->key );
//	}
//	else
//	{
//		printf( "%d!\n", c );
//	}

//	b = search( users, "lucia" );
//	printf( "%s - %d\n", b->nickname, b->key );



//	message_t *msg = NULL;
//	for( int i = 0; i < 30; i++ )
//	{
//		msg = (message_t *)malloc( sizeof( message_t ) );
//		setHeader( &msg->hdr, 200+i, "luca" );
//		char *tmp = (char *)malloc( 250 * sizeof( char ) );
//		strcpy( tmp, "CIAO MAMMA GUARDA COME MI DIVERTO" );
//		setData( &msg->data, b->nickname, tmp, strlen(tmp)+1 );
//		push( b->msg_hist, msg );
//	}
//
//	msg = b->msg_hist->tail->ptr;
//	printf( "%d\n", msg->hdr.op );
//	printf( "%s\n", msg->data.buf );
//    int r = 0;
//    while( r != 3 )
//    {
//    	sleep( 10 );
//    	r++;
//    }


//	printf( "\n\n\n\n" );
//    for( int i = 0; i < _MAX_CONN; i++ )
//	{
//    	user_t *tmp = NULL;
//		if( users->users[i].user != NULL )
//		{
//			 printf( "%s\n", users->users[i].user->nickname );
//		}
//		if( users->users[i].collision != NULL && users->users[i].collision->head != NULL )
//		{
//			node_t *nt = users->users[i].collision->head;
//			while( nt != NULL )
//			{
//				tmp = nt->ptr;
//				printf( "\t%s\n", tmp->nickname );
//				nt = nt->next;
//			}
//		}
//	}

	return 0;
}
