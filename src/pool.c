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
#include <src/queue.h>
#include <src/pool.h>
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
#include <signal.h>
#include <src/signup.h>


#define _POSIX_C_SOURCE >= 199506L || _XOPEN_SOURCE >= 500


int _MAX_CONN, _THREADn;	///< max_conn is number of max connection to handle
									///< num_thread is number of thread that operates in threadpool_t





/******************************************************************************
							MUTEX & CONDITION VARIABLES
******************************************************************************/
pthread_mutex_t lock_pool = PTHREAD_MUTEX_INITIALIZER;



/******************************************************************************
									FUNTIONS
******************************************************************************/
/**
 * @brief		function to extract from queue the first task added and run it
 * @var	pool	pointer to thread pool to can extract the task and its args
 */
void thread_work( threadpool_t *pool )
{
	while( pool->count >= 0 )
	{
		void (*tfun)(void *);
		void *arg_tfun;

		pthread_mutex_lock( &(pool->lock_t) );

		while( pool->count == 0 )
		{
			pthread_cond_wait( &(pool->cond_t), &(pool->lock_t) );
		}

		thread_task_t *task = NULL;
		if( (task = pull( pool->task )) == NULL )
		{
			perror( "pull" );
			fprintf( stderr, "Problem with tasks queue\n" );\
			return;
		}

		pool->count -= 1;

		tfun = task->function;
		arg_tfun = task->args;

		pthread_cond_signal( &(pool->cond_t) );
		pthread_mutex_unlock( &(pool->lock_t) );

//		( (task->function) (task->args) );
		tfun( arg_tfun );

		free( task );

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
	pool->queue_size = _MAX_CONN; ///< external variable
	pool->count = 0;
	pool->shutdown = 0;


	if( (pool->thread = ( pthread_t * )malloc( _THREADn * sizeof( pthread_t ) ) ) == NULL )
	{
		fprintf( stderr, "Problem to allocate space for thread" );
		exit( EXIT_FAILURE );
	}

	if( (pool->task = initialQueue() ) == NULL )
	{
		perror( "queue" );
		fprintf( stderr, "Problem to allocating space for tasks queue" );
		exit( EXIT_FAILURE );
	}


	pool->lock_t = ( pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
	pool->cond_t = ( pthread_cond_t )PTHREAD_COND_INITIALIZER;


	for( int i = 0; i < _THREADn; i++ )
	{
		if( pthread_create( &(pool->thread[i]), NULL, thread_work, pool ) != 0 )
		{
			threadpool_destroy( pool, 0 ); //TODO: destroy function
			fprintf( stderr, "Problem to create thread" );
			exit( EXIT_FAILURE );
		}
        pthread_detach(pool->thread[i]);

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
int threadpool_add( threadpool_t *pool, void(*functions)(void *), void *arg )
{
	if( pool == NULL || functions == NULL )
	{
		return -1;
	}

	if( pthread_mutex_lock( &(pool->lock_t) ) != 0 )
	{
		return -1;
	}

	if( pool->shutdown > 0 )
	{
		if( pthread_mutex_unlock( &(pool->lock_t) ) != 0 )
		{
			return -1;
		}
		return -1;
	}

	thread_task_t *tmp_t;
	if( (tmp_t = ( thread_task_t *)malloc( sizeof( thread_task_t ) )) == NULL )
	{
		perror( "malloc" );
		fprintf( stderr, "impossible add function to tasks queue" );
		return -1;
	}

	tmp_t->function = functions;
	tmp_t->args = arg;

	push( pool->task, tmp_t );

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
 * @brief		function to free queue of thread, queue of tasks and thread pool.
 * @var	pool	pointer to thread pool to be destroyed
 * @return	0	if all data structure is destroyed
 * 			-1	otherwise
 */
int threadpool_free( threadpool_t *pool )
{
    if( pool == NULL  )
    {
        return -1;
    }

    pthread_mutex_lock( &lock_pool );

    	destroy_queue( pool->task );

    	free( pool->thread );
    	pthread_cond_destroy( &(pool->cond_t) );
	pthread_mutex_unlock( &lock_pool );

	pthread_mutex_destroy( &(pool->lock_t) );

	free( pool );

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

    if( pthread_mutex_lock( &(pool->lock_t) ) != 0 )
    {
        err = -1;
    }

	//already shutting down
	if( pool->shutdown > 0 )
	{
		err = -1;
	}

	pool->shutdown = ( power_off && threadpool_graceful ) ? graceful_shutdown : immediate_shutdown;

	//wake up all worker threads
	if( pool->shutdown == 2 )
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

	if( pool->shutdown == 1 )
	{
		//kill all worker thread
		for( int i = 0; i < pool->thread_crt; i++ )
		{
			if( (err = pthread_kill( pool->thread[i], SIGCONT )) != 0 )
			{
				err = err * -1;
			}
		}
	}
	else
	{
		//join all worker thread
		for( int i = 0; i < pool->thread_crt; i++ )
		{
			if( pthread_join( pool->thread[i], NULL ) != 0 )
			{
				err = -1;
			}
		}
	}



    //only if all ok deallocates the pool
    if( err >= 0 )
    {
        threadpool_free( pool );
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
