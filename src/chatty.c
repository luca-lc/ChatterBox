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
#include <src/hashtable.h>

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

void print( void *arg )
{
	printf( "print: %d\n", (int)arg );
}

typedef struct complex_Arg
{
		int *a;
		int dim;
}arg;

void complex_op( void *args )
{
	arg *myargs = (arg*)args;
	for( int i = 0; i < myargs->dim; i++ )
	{
		int t = rand();
		myargs->a[i] = t;
	}

	for( int i = 0; i < myargs->dim; i++ )
	{
		print( myargs->a[i] );
	}
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

//
//	TEST
//

    //threadpool_t *p = pool_creation();

    hashtable_t *t = initTable( max_conn );
    printf( "prova %d", t->size);


    for( int i = 0; i < 5; i++ )
    {
    	char *c = ( char * )malloc( 20 * sizeof( char ) );
    	scanf( "%s", c );
    	insert( t, c );
    }

    printf( "elem: %d", t->n_elem );


    for( int i = 0; i < max_conn; i++ )
    {
    	printf( "\n\n%s", t->elem[i].nickname );
    	if( t->elem[i].collision != NULL )
    	{
    		for( node_t *n = t->elem[i].collision->head; n != NULL; n = n->next )
    		{
    			ht_elem_t *e = n->ptr;
    			printf( "\t%s\n",  e->nickname );
    		}
    	}
    }


    for( int i = 0; i < 5; i++ )
    {
    	char *c = ( char * )malloc( 20 * sizeof( char ) );
    	scanf( "%s", c );
    	printf( "%s found: %d", c, search( t, c ) );
    }






	return 0;
}