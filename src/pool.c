/* $pool.c$ */

/**
 * @file pool.c
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c)2017 Luca Canessa (516639)                                    *
 *                                                                            *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * @section DESCRIPTION
 * Looking overview
 *
 * In this file there are functions to create and manipulate threads pool
 *
 * thread_work( ) : Extracts task from queue in thread pool and run this
 * 					Requires pointer to a thread pool where is possible extract
 * 					a task and its arguments.
 *
 * pool_creation(): Creates a new threadpool_t thread pool type and initialize
 * 					its elements.
 * 					Returns pointer to new thread pool.
 *
 * threadpool_add():Adds a task in queue that be run later.
 * 					Requires pointer to thread pool where add the task, pointer
 * 					to function that represents task and pointer to its
 * 					arguments
 * 					Returns 0 if the task is added, else -1
 *
 * threadpool_free():Removes queues of thread and tasks and then destroy pool
 * 					Requires pointer to thread pool.
 * 					Returns 0 if remove them, else -1.
 *
 * threadpool_destroy():Stops all worker threads then remove all threads from
 * 					queue and call 'threadpool_free()' to remove other elements
 *
 * thread_stop():	Puts the 0 value at shutdown variable that used to stop threads
 *					Requires pointer to thread pool where threads to stop
 */



/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#include <./src/queue.h>
#include "./pool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>



/******************************************************************************
							MUTEX & CONDITION VARIABLES
******************************************************************************/
pthread_mutex_t lock_pool = PTHREAD_MUTEX_INITIALIZER;



/******************************************************************************
									FUNTIONS
******************************************************************************/
int max_conn = 20, num_thread = 20; //TODO: check variables



/**
 * @brief		function to extract from queue the first task added and run it
 * @var	pool	pointer to thread pool to can extract the task and its args
 */
void thread_work( threadpool_t *pool )
{
	thread_task_t tasks;

	int i = 0;
	while( pool->count > 0 )
	{
		pthread_mutex_lock( &(pool->lock_t) );
		while( pool->count == 0 )
		{
			pthread_cond_wait( &(pool->cond_t), &(pool->lock_t) );

		}
//	TODO: stop execution
//		if( pool->shutdown == 0)
//		{
//			pthread_mutex_unlock( &(pool->lock_t) );
//			printf( "shutdown\n" );
//			exit( EXIT_SUCCESS );
//		}

		int min = +INFINITY, min_i = -1;
		for( int i = 0; i < max_conn; i++ )
		{
			if( pool->task[i].next < min && pool->task[i].function != NULL )
			{
				min = pool->task[i].next;
				min_i = i;
			}
		}

		tasks.function = pool->task[min_i].function;
		tasks.args = pool->task[min_i].args;

		pool->task[min_i].function = NULL;
		pool->count -= 1;
		pthread_cond_broadcast( &(pool->cond_t) );
		pthread_mutex_unlock( &(pool->lock_t) );

		sleep(5);

		( *(tasks.function) )( tasks.args );
	}
}



/**
 * @brief			function to create and initialize a thread pool
 * @return	pool	pointer to thread pool created
 */
threadpool_t *pool_creation( )
{
	threadpool_t *pool;
	if( (pool = ( threadpool_t * )malloc( sizeof( threadpool_t ) )) == NULL )
	{
		fprintf( stderr, "Problem to allocate space for threadpool" );
		exit( EXIT_FAILURE );
	}

	// INITIALIZATION
	pool->thread_crt = 0;
	pool->queue_size = max_conn; ///< external variable
	pool->count = 0;


	if( (pool->thread = ( pthread_t * )malloc( num_thread * sizeof( pthread_t ) ) ) == NULL )
	{
		fprintf( stderr, "Problem to allocate space for thread" );
		exit( EXIT_FAILURE );
	}

	if( (pool->task = ( thread_task_t * )malloc( max_conn * sizeof( thread_task_t ) )) == NULL )
	{
		fprintf( stderr, "Problem to allocate space for queuetask" );
		exit( EXIT_FAILURE );
	}
	for( int i = 0; i < max_conn; i++ )
	{
		pool->task[i].function = NULL;
		pool->task[i].args = NULL;
		pool->task[i].next = -1;
	}
	pool->next_max = 0;


	pool->lock_t = ( pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
	pool->cond_t = ( pthread_cond_t )PTHREAD_COND_INITIALIZER;


	for( int i = 0; i < num_thread; i++ )
	{
		if( pthread_create( &(pool->thread[i]), NULL, thread_work, pool ) != 0 )
		{
			//threadpool_destroy( pool, 0 ); //TODO: destroy function
			fprintf( stderr, "Problem to create thread" );
			exit( EXIT_FAILURE );
		}
		pool->thread_crt += 1;
	}

	return pool;
}



/**
 * @brief			function to add task in queue to run by thread pool
 * @var	pool		pointer to thread pool where add task
 * @var	function	pointer to function that represents the task to run
 * @var	args		pointer to args of 'function' task
 * @return	0		if function terminates without issues
 * 			-1		otherwise
 */
int threadpool_add( threadpool_t *pool, void(*function)(void *), void *args )
{
	if( pool == NULL || function == NULL )
	{
		return -1;
	}

	if( pthread_mutex_lock( &(pool->lock_t) ) != 0 )
	{
		return -1;
	}

//	if( pool->count == pool->queue_size )
//	{
//		return -1;
//	}

	if( pool->shutdown )
	{
		return -1;
	}

	while( pool->count ==  max_conn )
	{
		fprintf( stderr, "\n\nWAIT: QUEUE IS FULL\n\n" ); //TODO: check wait condition and error message
		pthread_cond_wait( &(pool->cond_t), &(pool->lock_t) );

	}

	int i = 0;
	while( pool->task[i].function != NULL )
	{
		i++;
	}

	pool->task[i].function = function;
	pool->task[i].args = args;
	pool->task[i].next = pool->next_max += 1;
	pool->count += 1;

	if( pthread_cond_signal( &(pool->cond_t) ) != 0 )
	{
		return -1;
	}

	if( pthread_mutex_unlock( &(pool->lock_t) ) != 0 )
	{
		return -1;
	}


	return 0;
}



/**
 * @brief		function to free queue of thread, queue of tasks and thread pool. !!! CAUTION: run in safe mode, with mutex lock for all pool struct !!!
 * @var	pool	pointer to thread pool to be destroyed
 * @return	0	if all data structure is destroyed
 * 			-1	otherwise
 */
int threadpool_free( threadpool_t *pool )
{
    if(pool == NULL || pool->count > 0)
    {
        return -1;
    }

    pthread_mutex_lock( &lock_pool );
    if(pool->thread)
    {
        free(pool->thread);
        free(pool->task);

        pthread_mutex_destroy(&(pool->lock_t));
        pthread_cond_destroy(&(pool->cond_t));
    }

    free( pool );
    pthread_mutex_unlock( &lock_pool );
    return 0;
}



/**
 * @brief			function to stop worker thread immediately or after finished their jobs and destroy all pool
 * @var	pool		pointer to pool to destroy
 * @var	power_off	flag to stop worker, if 0 then stop immediately else stop when there are no more works
 * @return	err		0 if terminates without errors, else -1
 */
int threadpool_destroy( threadpool_t *pool, int power_off )
{
    int err = 0;

    if(pool == NULL)
    {
        err = -1;
    }

    if(pthread_mutex_lock(&(pool->lock_t)) != 0)
    {
        err = -1;
    }

	//already shutting down
	if(pool->shutdown)
	{
		err = -1;
	}

	pool->shutdown = ( power_off && threadpool_graceful ) ? graceful_shutdown : immediate_shutdown;

	//wake up all worker threads
	if( pool->shutdown )
	{
		if( pthread_cond_broadcast( &(pool->cond_t) ) != 0 )
		{
			err = -1;
		}
	}

	if( pthread_mutex_unlock( &(pool->lock_t) ) != 0 )
	{
		err = -1;
	}

	//join all worker thread
	for( int i = 0; i < pool->thread_crt; i++ )
	{
		if( pthread_join( pool->thread[i], NULL ) != 0 )
		{
			err = -1;
		}
	}


    //only if all ok deallocates the pool
    if( err != -1 )
    {
    	printf( "ropso1" );
        printf( "\nfree pool: %d\n", threadpool_free( pool ) );
    }


    return err;
}



/**
 * @brief		function to stop immediately all threads
 * @var	pool	pointer to the thread pool where is var to control shutting down
 */
void thread_stop( threadpool_t *pool )
{
	pthread_mutex_lock( &(pool->lock_t) );
	pool->shutdown = 0;
	pthread_mutex_unlock( &(pool->lock_t) );
}
