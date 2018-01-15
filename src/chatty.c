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
#include <src/config.h>
#include "src/chatty.h"

/* inserire gli altri include che servono */

/******************************************************************************
                                FUNCTIONS
******************************************************************************/
/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 }; //#registered user,#client online,#delivered mes,#to be delivered message,#delivered files, #to be delivered files, #errors messages



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
void print_statistic( FILE *stat )
{
	int r = 0;
	while( true && r == 0 )
	{
		r = printStats( stat );
		sleep( 30 );
	}
}



/**
 *
 */
void initServer( struct s_conf *config )
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

	strcpy( Dir, config->DirName );
}



/**
 *
 */
void mng_conn( int arg )
{
	int c_fd = (int)arg;
	printf( "Connection management has been started\n" );
	printf( "--------------------------------------\n\n" );
	message_t msg;
	msg.data.hdr.len = 0;
	if( readMsg( arg, &msg ) <= 0 )
	{
		fprintf( stderr, "problem to receive" );
	}

	int end = strlen(msg.data.buf);
	while( msg.data.buf[end] != '/' )
		end--;
	end++;
	char tmp_F[256];
	strcpy( tmp_F, Dir );
	strcat( tmp_F, msg.data.buf+end );
	printf( "> %s\n\n\n", tmp_F );

	FILE* ffd;
	int i = 1;
	if( msg.hdr.op == POSTFILE_OP )
	{
		ffd = fopen( tmp_F,  "w+" );
		if( ffd == NULL )
		{
			printf( "FILE NON APERTO\n" );
		}
	}

	message_data_t data;
	readData( arg, &data );

	fwrite( data.buf, sizeof( char ), data.hdr.len, ffd );





	fclose(ffd);
	close(arg);

	printf( "OP:\n > %d\nSENDR:\n > %s\n", msg.hdr.op, msg.hdr.sender );
	printf( "LEN:\n > %d\nRECR:\n > %s\n", msg.data.hdr.len, msg.data.hdr.receiver );
	printf( "BODY:\n > %s\n", msg.data.buf );
	printf( "######################################\n\n" );

}




/******************************************************************************
                                    MAIN
******************************************************************************/
int main(int argc, char *argv[])
{
    /** VARIABLES **/
    FILE *stats;
    struct s_conf myconf;
    int stat_o_time = 5;

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
//    while( (stats = fopen( myconf.StatFileName, "a+" )) == NULL && stat_o_time > 0 )
//    {
//    	perror( "fopen()" );
//    	fprintf( stderr, "%s\n\n", myconf.StatFileName );
//    	fprintf( stderr, "Try again %d times\n",  stat_o_time );
//    	stat_o_time --;
//    }
//    if( stat_o_time == 0 )
//    {
//    	fprintf( stderr, "Unable to open the file %s\n", myconf.StatFileName );
//    }
//    pthread_t stat_printer;
//    pthread_create( &stat_printer, NULL, (void *)print_statistic, (void *)stats );


    initServer( &myconf );



    //TEST

    unlink(SOCKNAME);

//    threadpool_t *mypool = pool_creation( );
//
//    int s_fd, c_fd;
//
//    struct sockaddr_un se;
//    strncpy( se.sun_path, SOCKNAME, UNIX_PATH_MAX );
//    se.sun_family = AF_UNIX;
//
//    s_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
//    bind( s_fd, (struct sockaddr *)&se, sizeof(se) );
//    listen( s_fd, _MAX_CONN );
//
//    while( true )
//    {
//    	c_fd = accept( s_fd, NULL, 0 );
//    	if( c_fd != -1 )
//    	{
//    		threadpool_add( mypool, mng_conn, (int)c_fd );
//    	}
//    	else
//    	{
//    		fprintf( stderr, "ERROR ESTABLISHING CONNECTION\n");
//    		continue;
//    	}
//    }




    hashtable_t *users = initTable( _MAX_CONN );

    checkin_arg in;
    in.myt = users;

    in.name = "Luca";
    checkin( &in );

	in.name = "Roby";
	checkin( &in );

	in.name = "Betta";
	checkin( &in );

	ht_elem_t *b = search( users, "Betta" );
	message_t *msg = NULL;
	for( int i = 0; i < 30; i++ )
	{
		msg = (message_t *)malloc( sizeof( message_t ) );
		setHeader( &msg->hdr, 200+i, "luca" );
		char *tmp = (char *)malloc( 250 * sizeof( char ) );
		strcpy( tmp, "CIAO MAMMA GUARDA COME MI DIVERTO" );
		setData( &msg->data, b->nickname, tmp, strlen(tmp)+1 );
		push( b->msg_hist, msg );
	}

	msg = b->msg_hist->tail->ptr;
	printf( "%d\n", msg->hdr.op );
	printf( "%s\n", msg->data.buf );

	return 0;
}
