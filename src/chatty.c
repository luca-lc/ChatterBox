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
hashtable_t *users;

struct arg_mng
{
	hashtable_t *u;
	int con_fd;
};
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
void mng_conn( int c_fd )
{
	message_t msg;

	printf( "Connection management has been started\n" );
	printf( "--------------------------------------\n\n" );
	int i = 1;
	while( msg.hdr.op != DISCONNECT_OP )
	{
		printf("LOOP: %d\n", i );
		readMsg( c_fd, &msg );

		switch( msg.hdr.op )
		{
			case REGISTER_OP:
			{
				int ack_reg;

				ack_reg = checkin( users, msg.hdr.sender );

				if( ack_reg == true )
				{
					setHeader( &msg.hdr, OP_OK, "ChattyServer" );
				}
				else
				{
					switch( ack_reg )
					{
						case false: setHeader( &msg.hdr, OP_FAIL, "ChattyServer" );
								break;
						case OP_NICK_ALREADY: setHeader( &msg.hdr, OP_NICK_ALREADY, "ChattyServer" );
								break;
					}

				}

				sendHeader( c_fd, &msg.hdr );
			}break;
//			case CONNECT_OP:
//			{
////				ht_elem_t *user_con = search( users, msg.hdr.sender );
////				if( user_con != NULL )
////					printf( "%s\n", user_con->nickname );
////				else
////					printf( "PUPPONE\n" );
//			}break;
//			case POSTTXT_OP:
//			{
//				message_t *new_m = (message_t *)malloc( sizeof( message_t ) );
//
//			}break;
			default: printf("PUPPA\n"); break;

		}
		i++;
	}
	printf( "######################################\n\n" );

	close( c_fd );

}




/******************************************************************************
                                    MAIN
******************************************************************************/
int main(int argc, char *argv[])
{
    /** VARIABLES **/
    struct s_conf myconf;

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
    		threadpool_add( pool, mng_conn, c_fd );
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
