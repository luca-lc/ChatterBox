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




void mng_conn( int arg )
{
	//int c_fd = (int)arg;
	printf( "Connection management has been started...\n" );
	message_t msg;
//	char buff[50];
//	char tmp[256];

	readHeader( arg, &msg.hdr );
	readData( arg, &msg.data );

	close(arg);
//	recv( arg, buff, 50, 0 );
	printf( "\nOP: %d\tSEN: %s\tRECV: %s\tLEN: %d\n", msg.hdr.op, msg.hdr.sender, msg.data.hdr.receiver, msg.data.hdr.len );
	printf( "\n%s\n", msg.data.buf );
	sleep( 3 );
}




/******************************************************************************
                                    MAIN
******************************************************************************/
int main(int argc, char *argv[])
{
    /** VARIABLES **/
    FILE *stats;



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
                if( (stats=fopen( "./.statschat.log", "a+" )) == NULL )
                {
                    fprintf( stderr, "problem to create or open stats file" );//TODO: check
                }
                else
                {
                    printStats( stats ); //TODO: clean "statschat.log"
                }
            }
                break;

            default: ;
        }
    }


//TEST

    unlink(SOCKNAME);

    threadpool_t *mypool = pool_creation( );

    int s_fd, c_fd;

    struct sockaddr_un se;
    strncpy( se.sun_path, SOCKNAME, UNIX_PATH_MAX );
    se.sun_family = AF_UNIX;

    s_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    bind( s_fd, (struct sockaddr *)&se, sizeof(se) );
    listen( s_fd, max_conn );

    while( true )
    {
    	c_fd = accept( s_fd, NULL, 0 );
    	if( c_fd != -1 )
    	{
    		threadpool_add( mypool, mng_conn, (int)c_fd );
    	}
    	else
    	{
    		fprintf( stderr, "ERROR ESTABLISHING CONNECTION\n");
    		continue;
    	}
    }


    //fprintf( stdout, "\nOK FATTO\n" );
	return 0;
}
