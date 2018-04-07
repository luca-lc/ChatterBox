/** $pool_c$ **/

/**
 * @file pool.c
 * @section LICENSE
 * ****************************************************************************
 * @author Luca Canessa (516639)                                   			  *
 *        																	  *
 * @copyright \n                                                              *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * @section DESCRIPTION
 * Looking overview
 *
 * In this file there are body of functions to create and manipulate threads 
 * pool. The threads pool was designed like a table where are saved the pointer
 * to array of ID threads, pointer to queue where will puts the tasks to be 
 * solved, mutual exclusion variable, condition variable, counter of tasks to 
 * do and a flag that determiny if threads shall be killed or shall be 
 * terminated after finished their works.  The tasks scheduler was designed to 
 * assign one task at time per thread and after a thread finish its job, if the 
 * tasks queue is not empty takes the first element in head of queue, otherwise
 * the condition variable puts scheduler in pause until that a task will be 
 * queued. So the scheduler is a FIFO queue.
 *
 * thread_work() : 	If the counter of the activities in the queue is greater 
 * 					than zero, it extracts the activity (the function and its 
 * 					arguments) located in the head of the queue that must be 
 * 					executed by a thread, otherwise it is paused. It also 
 * 					updates the activity counter
 * 					Requires pointer to a thread pool where is possible extract
 * 					a task and its arguments.
 * 					Returns nothing.
 *
 * pool_creation(): Allocates space for new thread pool and initializes all its 
 * 					items. (It allocates space for threads, for tasks queue and 
 * 					start each threads required and initializes all variables).
 * 					The number of threads in thread pool is taken from config 
 * 					file.
 * 					Requires nothing
 * 					Returns pointer to new thread pool.
 *
 * threadpool_add():Adds a task (function and its arg) to queue, increases activity 
 * 					counter and unblocks held threads on conditional variable
 * 					Requires pointer to thread pool, pointer to function and pointer to arg
 * 					Returns 0 if terminate without errors, else -1
 *
 * threadpool_free():Removes queued task, thread and then destroys pool
 * 					Requires pointer to thread pool.
 * 					Returns 0 if terminate without errors, else -1.
 *
 * threadpool_destroy():Stops all worker threads, the stop mode depends on 
 * 						flag sent, then remove all threads and call 
 * 						'threadpool_free()' to remove other elements.
 * 						Requires pointer to thread pool
 * 						Returns 0 if terminates without errors, else -1
 */



/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
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

#include <src/queue.h>
#include <src/pool.h>
#include <src/signup.h>


#define _POSIX_C_SOURCE >= 199506L || _XOPEN_SOURCE >= 500


int _MAX_CONN;	///< _MAX_CONN is number of max connections to handle
int _THREADn;	///< _THREADn is number of thread that operates in threadpool_t



/******************************************************************************
									FUNTIONS
******************************************************************************/
/**
 * @function				thread_work
 * @brief			It extracts from tasks queued the first, update the tasks 
 * 					counter and at end run the activity. The tasks represents 
 * 					function. This function keeps active until the thread where 
 * 					is located it keep alive
 * @param	pool	Pointer to thread pool of type 'threadpool_t'
 */
void thread_work( threadpool_t *pool )
{
	while( 1 )
	{
		void (*tfun)(void *);
		void *arg_tfun;

		pthread_mutex_lock( &(pool->lock_t) );
		
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		while( pool->count == 0 && pool->shutdown != 1 )
		{
			pthread_cond_wait( &(pool->cond_t), &(pool->lock_t) );
		}

		if( pool->shutdown == 1 )
			{
				pthread_exit( NULL );
				return;
			}
		else
			{
				thread_task_t *task = NULL;
				if( (task = pull( pool->task )) == NULL )
				{
					perror( "pool" );
					fprintf( stderr, "Problem with tasks queue\n" );
					continue;
				}

				pool->count -= 1;


				tfun = task->function;
				arg_tfun = task->args;

				pthread_cond_signal( &(pool->cond_t) );
				pthread_mutex_unlock( &(pool->lock_t) );

				tfun( arg_tfun );

				free( task );
				pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

			}
	}
}



/**
 * @function				pool_creation
 * @brief			It allocates space to create a new thread pool pf type
 * 					'threadpool_t'. It allocates space for threads and tasks,
 * 					create threads and initializes all variables. To allocate 
 * 					space for tasks uses 'initialQueue()', to create threads 
 * 					uses 'malloc()' for IDs and 'pthread_create()' to create them.
 * 					The number of thread in the pool is taken from config file
 * 					from string 'ThreadsInPool'.
 * @return	pool	Pointer to thread pool created.
 */
threadpool_t *pool_creation( )
{
	threadpool_t *pool;
	if( (pool = ( threadpool_t * )calloc( 1, sizeof( struct pool ) )) == NULL )
	{
		fprintf( stderr, "Problem to allocate space for threadpool" );
		return NULL;
	}

	// INITIALIZATION
	pool->count = 0;
	pool->shutdown = 0;


	if( (pool->thread = ( pthread_t * )calloc( _THREADn, sizeof( pthread_t ) ) ) == NULL )
	{
		fprintf( stderr, "Problem to allocate space for thread" );
		free( pool );
		return NULL;
	}

	if( (pool->task = initialQueue() ) == NULL )
	{
		fprintf( stderr, "Problem to allocating space for tasks queue" );
		free( pool->thread );
		free( pool );
		return NULL;
	}


	pool->lock_t = ( pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
	pool->cond_t = ( pthread_cond_t )PTHREAD_COND_INITIALIZER;


	for( int i = 0; i < _THREADn; i++ )
	{
		if( pthread_create( &(pool->thread[i]), NULL, (void *)thread_work, (void *)pool ) != 0 )
		{
			threadpool_destroy( pool, 0 );
			fprintf( stderr, "Problem to create thread" );
			free( pool->thread );
			free( pool->task );
			free( pool );
			
			return NULL;
		}
		pthread_detach( pool->thread[i] );
	}

	return pool;
}



/**
 * @function					threadpool_add
 * @brief				It adds a task (function and its arg) to queue, increases 
 * 						activity counter and unblocks held threads on conditional 
 * 						variable. (Some operations MUST done in safe condition,
 * 						with mutex acquired)
 * @param	pool		Pointer to the thread pool where add task
 * @param	function	Pointer to the function to add
 * @param	args		Pointer to the args of this function
 * @return				0 if it exits witout errors, -1 otherwise
 */
int threadpool_add( threadpool_t *pool, void(*functions)(void *), void *arg )
{
	if( pool == NULL || functions == NULL )
	{
		return -1;
	}

	if( pool->shutdown > 0 )
	{
		if( pthread_mutex_unlock( &(pool->lock_t) ) != 0 )
		{
			return -1;
		}
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


	if( pthread_mutex_lock( &(pool->lock_t) ) != 0 )
	{
		return -1;
	}

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
 * @function				threadpool_free
 * @brief			It removes queued task, thread and then destroys pool.
 * 					(Some operation MUST be done in safe mode, with mutex 
 * 					acquired)
 * @param	pool	Pointer to the thread pool to be cleaned
 * @return			0 if it exits, -1 if 'pool' is NULL 
 */
int threadpool_free( threadpool_t *pool )
{
    if( pool == NULL  )
    {
        return -1;
    }

    pthread_mutex_lock( &pool->lock_t );

    	destroy_queue( pool->task );

    	free( pool->thread );
    	pthread_cond_destroy( &(pool->cond_t) );
	pthread_mutex_unlock( &pool->lock_t );

	pthread_mutex_destroy( &(pool->lock_t) );

	free( pool );

    return 0;
}



/**
 * @function					threadpool_destroy
 * @brief				It stops all worker threads, the stop mode depends on 
 * 						flag sent, then remove all threads and call 
 * 						'threadpool_free()' to remove other elements.
 * 						Some operations MUST be executed with mutex acquired
 * @param	pool		Pointer to the thread pool to be destroyed
 * @param	power_off	Flag to stop worker, if 0 then stop immediately else 
 * 						stop when there are no more works
 * @return	err			0 if terminates without errors, else -1
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

	pool->shutdown = power_off;//( power_off && threadpool_graceful ) ? graceful_shutdown : immediate_shutdown;

	//wake up all worker threads
	// if( pool->shutdown == 2 )
	// {
	// 	if( pthread_cond_broadcast( &(pool->cond_t) ) != 0 )
	// 	{
	// 		err = -1;
	// 	}
	// }

	if( pthread_mutex_unlock( &(pool->lock_t) ) != 0 )
	{
		err = -1;
	}

	if( pool->shutdown == 1 )
	{
		//kill all worker thread
		pthread_cond_broadcast( &(pool->cond_t) );
	}
	else
	{
		//join all worker thread
		for( int i = 0; i < _THREADn; i++ )
		{
			if( (err = pthread_cancel( pool->thread[i] )) != 0 )
			{
				err = -1;
			}
		}
	}


    //only if all ok deallocates the pool
    
    threadpool_free( pool );


    return err;
}
