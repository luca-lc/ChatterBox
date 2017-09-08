/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#include "./pool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>



/******************************************************************************
 	 	 	 	 	 	 	 	 CONDITION & LOCK
******************************************************************************/
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;



/******************************************************************************
									FUNTIONS
******************************************************************************/

/** =============== SEMAPHORES =============== **/
/**
 * @brief		initializes binary semaphore to control jobs queue
 * @param	S	pointer to semaphore to be initialized
 * @param	val value to gives to busy semaphore
 */
void sem_init( sem_t *S, int val )
{
	if( val < 0 || val > 1 )
	{
		fprintf( stderr, "sem_init(): binary semaphore, take only 0 or 1." );
		exit( 1 );
	}
	S->s_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	S->s_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	S->s_v = val;
}



/**
 * @brief		resets semaphore using 'sem_init'
 * @param	S	pointer to semaphore to be reset
 */
void sem_reset( sem_t *S )
{
	sem_init( S, 0 );
}



/**
 * @brief		sets semaphore value at 1 and frees another semaphore in wait queue
 * @param	S	pointer to semaphore to changes its value
 */
void sem_post( sem_t *S )
{
	pthread_mutex_lock( &S->s_mutex );
	S->s_v = 1;
	pthread_cond_signal( &S->s_cond );
	pthread_mutex_unlock( &S->s_mutex );
}



/**
 * @brief		sets semaphore value at 1 and frees all other semaphore in wait queue
 * @param	S	pointer to semaphore to changes its value
 */
void every_sem_post( sem_t *S )
{
	pthread_mutex_lock( &S->s_mutex );
	S->s_v = 1;
	pthread_cond_broadcast( &S->s_cond );
	pthread_mutex_unlock( &S->s_mutex );
}



/**
 * @brief		checks if semaphore is set at 1 then resets its value else waits until changed its value
 * @param	S	pointer to semaphore to be checked
 */
void sem_wait( sem_t *S )
{
	pthread_mutex_lock( &S->s_mutex );
	while( S->s_v != 1 )
	{
		pthread_cond_wait( &S->s_cond, &S->s_mutex );
	}
	S->s_v = 0;
	pthread_mutex_unlock( &S->s_mutex );
}

/** =============== JOBS =============== **/
/**
 * @brief		initializes the jobs queue
 */
int init_queue_jobs( queue_jobs_t *qj )
{
	qj->len = 0;
	qj->head = NULL;
	qj->tail = NULL;

	qj->has_jobs = ( sem_t * )malloc( sizeof( sem_t ) );
	if( qj->has_jobs == NULL )
	{
		return -1;
	}

	qj->qj_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

	sem_init( qj->has_jobs, 0 );

	return 0;
}



/*
 *
 */
void clear_jobs_queue( queue_jobs_t *qj )
{
	while( qj->len )
	{
		free( pull_jobs_queue(qj) );
	}

	qj->head = NULL;
	qj->tail = NULL;
	sem_reset( qj->has_jobs );
	qj->len = 0;
}



/*
 *
 */
void push_jobs_queue( queue_jobs_t *qj, jobs_t *n_j )
{
	pthread_mutex_lock( &qj->qj_lock );
	n_j->prev = NULL;

	switch( qj->len )
	{
		case 0:
			qj->head = n_j;
			qj->tail = n_j;
			break;

		default:
			qj->tail->prev = n_j;
			qj->tail = n_j;
	}
	qj->len += 1;

	sem_post( qj->has_jobs );

	pthread_mutex_unlock( &qj->qj_lock );
}



/*
 *
 */
jobs_t *pull_jobs_queue( queue_jobs_t *qj )
{
	pthread_mutex_lock( &qj->qj_lock );
	jobs_t *tmp_j = qj->head;

	switch( qj->len )
	{
		case 0:
			break;

		case 1:
			qj->head = NULL;
			qj->tail = NULL;
			qj->len = 0;
			break;

		default:
			qj->head = tmp_j->prev;
			qj->len -= 1;
			sem_post( qj->has_jobs );
	}

	pthread_mutex_unlock( &qj->qj_lock );

	return tmp_j;
}



/*
 *
 */
void erase_jobs_queue( queue_jobs_t *qj )
{
	clear_jobs_queue( qj );
	free( qj->has_jobs );
}



/*
 *
 */


