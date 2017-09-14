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
#include <time.h>

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

//void print( void *arg )
//{
//	printf( "print: %d\n", (int)arg );
//}

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


/* ====================== TEST QUEUE =============================== */
    /* ++++++++++++++++++++++++++ ALL OK +++++++++++++++++++++++++++++++ */
//    queue_t *r = initialQueue( );
//
//    push( r, (int)2);
//    push( r, (int)12);
//    push( r, (int)3);
//    push( r, (int)5);
//
//    clear_queue( r );
//
//    printf( "%d\n", (int)pull(r) );
//    printf( "%d\n", (int)pull(r) );
//    printf( "%d\n", (int)pull(r) );
    /* ++++++++++++++++++++++++++ ALL OK +++++++++++++++++++++++++++++++ */


/* ====================== TEST THREAD_WORK WITHOUT & WITH THREAD =============================== */

    /* ++++++++++++++++++++++++++ ALL OK +++++++++++++++++++++++++++++++ */
//    threadpool_t *pool = ( threadpool_t * )malloc( sizeof( threadpool_t ) );
//    pool->lock_t = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
//    pool->cond_t = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
//    pool->count = pool->queue_size = 3;
//    pool->head = 0;
//    pool->tail = 2;
//    pool->task = (thread_task_t *)malloc( 3 * sizeof(thread_task_t));
//    for( int i = 0; i < 3; i++ )
//	{
//    	pool->task[i].function = print;
//    	pool->task[i].args = i+3;
//	}
//
//    //thread_work( pool );   //WHITOUT THREAD
//
//    pool->thread = (pthread_t *)malloc( 2 * sizeof(pthread_t));
//    for( int j = 0; j < 2; j++ )
//    {
//    	pthread_create( &(pool->thread[j]), NULL, thread_work, pool );
//    }
//
//    usleep( 10000 );
//
//    for( int j = 0; j < 2; j++ )
//	{
//		pthread_join( &(pool->thread[j]), NULL );
//	}
    /* ++++++++++++++++++++++++++ ALL OK +++++++++++++++++++++++++++++++ */

/* ====================== TEST POOL_CREATION =============================== */

    /* ++++++++++++++++++++++++++ ALL OK +++++++++++++++++++++++++++++++ */
//    threadpool_t *p = pool_creation();
//
//    usleep( 10000 );
//    printf( "\n\n\n" );
//    printf( "thread_crt: \t %d \n", p->thread_crt );
//    printf( "queue_size: \t %d \n", p->queue_size );
//    printf( "head: \t %d \n", p->head );
//    printf( "tail: \t %d \n", p->tail );
//    printf( "count: \t %d \n", p->count );
//    printf( "shutdown: \t %d \n", p->shutdown );
//    printf( "started: \t %d \n", p->started );
    /* ++++++++++++++++++++++++++ ALL OK +++++++++++++++++++++++++++++++ */


/* ====================== TEST THREADPOO_ADD =============================== */

    /* ++++++++++++++++++++++++++ ALL OK +++++++++++++++++++++++++++++++ */
//    threadpool_t *pool = (threadpool_t *)malloc(max_conn * sizeof( threadpool_t ));
//    pool->queue_size = max_conn;
//    pool->head = pool->tail = pool->count = 0;
//    pool->shutdown = 0;
//    pool->started = 0;
//    pool->task = ( thread_task_t * )malloc( max_conn * sizeof( thread_task_t ));
//    for( int i = 0; i < max_conn; i++ )
//    {
//    	threadpool_add( pool, print, i+250 );
//    }
//
//
//    for( int j = 0; j < max_conn; j++ )
//    {
//    	printf( "arg[%d] %d\n", j, pool->task[j].args );
//    }
    /* ++++++++++++++++++++++++++ ALL OK +++++++++++++++++++++++++++++++ */



/* ====================== TEST THREADPOOL =============================== */

//    threadpool_t *p = pool_creation();
//
//
//    for( int i = 0; i < max_conn; i++ )
//    {
//    	threadpool_add( p, print, i );
//    }
//
//
//
//
//    usleep( 100000 );


    return 0;
}
