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
									FUNTIONS
******************************************************************************/
int max_conn = 4, num_thread = 20;



/**
 * @brief		function to extract from queue the first task added and run it
 * @var	args	pointer to thread pool to can extract the task and its args
 */
void thread_work( void *args )
{
	threadpool_t *pool = ( threadpool_t * )args;

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
	pool->queue_size = max_conn; //external variable
	pool->count = 0;
	pool->shutdown = pool->started = 0;


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
		pool->started += 1;
		pool->thread_crt += 1;
	}

	return pool;
}


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
		fprintf( stderr, "\n\nWAIT: QUEUE IS FULL\n\n" );
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
	printf( "%d\n", pool->count );

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

/*
int threadpool_free(threadpool_t *pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    //Did we manage to allocate ?
    if(pool->thread) {
        free(pool->thread);
        free(pool->task);

        // Because we allocate pool->threads after initializing the
        //   mutex and condition variable, we're sure they're
        //   initialized. Let's lock the mutex just in case.
        pthread_mutex_lock(&(pool->lock_t));
        pthread_mutex_destroy(&(pool->lock_t));
        pthread_cond_destroy(&(pool->cond_t));
    }
    free(pool);
    return 0;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    int i, err = 0;

    if(pool == NULL) {
        return -1;
    }

    if(pthread_mutex_lock(&(pool->lock_t)) != 0) {
        return -1;
    }

    do {
        //Already shutting down
        if(pool->shutdown) {
            err = -1;
            break;
        }

        pool->shutdown = (flags && threadpool_graceful) ?
            graceful_shutdown : immediate_shutdown;

        //Wake up all worker threads
        if((pthread_cond_broadcast(&(pool->cond_t)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock_t)) != 0)) {
            return -1;
        }

        //Join all worker thread
        for(i = 0; i < pool->thread_crt; i++) {
            if(pthread_join(pool->thread[i], NULL) != 0) {
                return -1;
            }
        }
    } while(0);

    //Only if everything went well do we deallocate the pool
    if(!err) {
        threadpool_free(pool);
    }
    return err;
}



void thread_stop( threadpool_t *pool )
{
	pthread_mutex_lock( &(pool->lock_t) );
	pool->shutdown = 0;
	pthread_mutex_unlock( &(pool->lock_t) );
}
*/
