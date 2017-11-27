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
//#include <src/signup.h>
#include <src/ops.h>
#include <src/pool.h>
#include <time.h>
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


//    threadpool_t *p = pool_creation();
//
//    hashtable_t *t = initTable( max_conn );
//
//    checkin_arg *ar = ( checkin_arg *)malloc(sizeof( checkin_arg ) );
//    ar->name = (char *)malloc( 20 * sizeof( char ) );
//	ar->myt = t;
//
//	for( int i = 0; i < 10; i++ )
//	{
//		printf( "inserisci\t" );
//		gets( ar->name );
//		threadpool_add( p, checkin, ar );
//	}
//
//	printf( "\n\n\n" );
//	sleep( 2 );
//
//	for( int i = 0; i < max_conn; i++ )
//	{
//		if( t->elem[i].nickname != NULL )
//		{
//			printf( "%s\n", t->elem[i].nickname );
//
//			if( t->elem[i].collision != NULL )
//			{
//				node_t *n = t->elem[i].collision->head;
//				while( n != NULL )
//				{
//					ht_elem_t *e = n->ptr;
//					printf( "\t\t%s\n", e->nickname );
//					n = n->next;
//				}
//			}
//		}
//
//	}
//
//	for( int i = 0; i < 4; i++ )
//	{
//		printf( "\nrimuovi?\n" );
//		gets( ar->name );
//
//
//		printf( "%d\n",  delete( ar ) );
//
//
//		for( int i = 0; i < max_conn; i++ )
//		{
//			if( t->elem[i].nickname != NULL )
//			{
//				printf( "%s\n", t->elem[i].nickname );
//
//				if( t->elem[i].collision != NULL )
//				{
//					node_t *n = t->elem[i].collision->head;
//					while( n != NULL )
//					{
//						ht_elem_t *e = n->ptr;
//						printf( "\t\t%s\n", e->nickname );
//						n = n->next;
//					}
//				}
//			}
//
//		}
//	}
//

//
//
//    queue_t *myq = initialQueue();
//    push( myq, "luca" );
//    push( myq, "lucia" );
//    push( myq, "luciano" );
//
//    node_t *tmp = myq->head;
//    while( tmp != NULL )
//    {
//    	printf( "ORIGINAL: %s\t", tmp->ptr );
//    	tmp = tmp->next;
//    }
//
//    printf("\n\n\nOUT: %s\n\n", pull( myq ) );
//
//    tmp = myq->head;
//    while( tmp != NULL )
//	{
//		printf( "LAST: %s\t", tmp->ptr );
//		tmp = tmp->next;
//	}
//
//


    hashtable_t *tb = initTable( 4 );

    insert( tb, "luca" );
    insert( tb, "ciao" );
//
    printf( "%d\n", search( tb, "luca" ) );
    printf( "%d\n", search( tb, "ciao" ) );
    printf( "%d\n", search( tb, "pippo" ) );



	fprintf( stdout, "\nOK FATTO\n" );
	return 0;
}
