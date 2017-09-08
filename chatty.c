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
#include <stats.h>
#include <queue.h>
#include <sign_up.h>
#include <ops.h>
#include <pool.h>

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


    /** =========== TEST =========== **/

    queue_t *r = initialQueue( );

    push( r, (int)2);
    push( r, (int)12);
    push( r, (int)3);
    push( r, (int)5);

    printStats( stats );


    sem_t s;
    sem_init( &s, 1 );
	printf( "\t*%d* rosp1o", s.s_v );

	sem_reset( &s );
	printf( "\t*%d* rosp1o", s.s_v );

	sem_post( &s );
	printf( "\t*%d* rosp1o", s.s_v );

	sem_wait( &s );
	printf( "\t*%d* rosp1o", s.s_v );

	//sem_wait( &s );
	printf( "\t*%d* rosp1o", s.s_v );


n

    return 0;
}
