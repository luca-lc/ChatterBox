/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#include "./pool.h"



/******************************************************************************
 	 	 	 	 	 	 	 	 CONDITION & LOCK
******************************************************************************/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;



/******************************************************************************
									FUNTIONS
******************************************************************************/
/* ================ SEMAPHORES ================ */
/*
 *
 */
void sem_init( sem_t *S, int val )
{
	if( val < 0 || val > 1 )
	{
		fprintf( stderr, "sem_init(): binary semaphore, take only 0 or 1." );
		exit( 1 );
	}

	S->s_mutex = PTHREAD_MUTEX_INITIALIZER;
	S->s_cond = PTHREAD_COND_INITIALIZER;
	S->s_v = val;
}



/*
 *
 */
void sem_reset( sem_t *S )
{
	sem_init( S, 0 );
}



/*
 *
 */
void sem_post( sem_t *S )
{
	pthread_mutex_lock( &S->s_mutex );
	S->s_v = 1;
	pthread_cond_signal( &S->s_cond );
	pthread_mutex_unlock( &S->s_mutex );
}



/*
 *
 */
void every_sem_post( sem_t *S )
{
	pthread_mutex_lock( &S->s_mutex );
	S->s_v = 1;
	pthread_cond_broadcast( &S->s_cond );
	pthread_mutex_unlock( &S->s_mutex );
}



/*
 *
 */
void sem_wait( sem_t *S )
{
	pthread_mutex_lock( &S->s_mutex );
	while( S->s_v != 1 )
	{
		pthread_cond_wait( &S->s_cond, &S->s_mutex );
	}
	sem->s_v = 0;
	pthread_mutex_unlock( &S->s_mutex );
}

/* ================ JOBS ================ */
/*
 *
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

	qj->qj_lock = PTHREAD_MUTEX_INITIALIZER;

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
		free( jobs_queue_push(qj) );
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
	n_j->head = NULL;

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


