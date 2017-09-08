#ifndef _POOL_H_
#define _POOL_H_



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


extern int num_pool;


/******************************************************************************
									STRUCTURES
******************************************************************************/
/*
** SEMAPHORES **
*/
typedef struct sem
{
	pthread_mutex_t s_mutex;
	pthread_cond_t s_cond;
	int s_v;
}sem_t;

/*
 **	JOB **
*/
typedef struct job
{
	struct job *prev;					//pointer to previous job
	void ( * funct )( void *arg ); 		//function pointer
	void *arg;							//pointer to function argument
}jobs_t;


/*
 ** JOB QUEUE **
*/
typedef struct qj
{
	pthread_mutex_t qj_lock;			//mutex for queue access
	jobs_t *head;						//pointer to qj queue head
	jobs_t *tail;						//pointer to qj queue tail
	sem_t *has_jobs;					//flag used like semaphore
	int len;							//number of jobs in queue
}queue_jobs_t;


/*
 ** THREAD POOL
*/
typedef struct thpool
{
	struct th **threads;					//pointer to threads
	volatile int active_th;				//number of threads at work
	volatile int th_set;				//number of thread alive
	pthread_mutex_t pool_lock;
	pthread_cond_t	pool_cond;			//condition for thread to keep wait
	queue_jobs_t *jobs;					//pointer to queue jobs
} th_pool_t;


/*
 **	THREAD **
*/
typedef struct th
{
	int tid;							//thread id
	pthread_t thread;					//pointer to thread
	struct thpool *tpool;				//pointer to thread pool
}thread_t;



/******************************************************************************
									FUNCTIONS
******************************************************************************/

/** =============== SEMAPHORES =============== **/
/*
  @
*/
void sem_init( sem_t *S, int val );



/*
  @
*/
void sem_reset( sem_t *S );



/*
  @
*/
void sem_post( sem_t *S );



/*
  @
*/
void every_sem_post( sem_t *S );



/*
  @
*/
void sem_wait( sem_t *S );



/** =============== JOBS =============== **/
/*
  @
*/
int init_queue_jobs( queue_jobs_t *qj );



/*
  @
*/
void clear_jobs_queue( queue_jobs_t *qj );



/*
  @
*/
void push_jobs_queue( queue_jobs_t *qj, jobs_t *n_j );



/*
  @
*/
jobs_t *pull_jobs_queue( queue_jobs_t *qj );



/*
  @
*/
void erase_jobs_queue( queue_jobs_t *qj );


#endif
