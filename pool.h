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


extern int num_pool = 5;


/******************************************************************************
									STRUCTURES
******************************************************************************/
/*
** SEMAPHORES **
*/
typedef struct sem
{
	pthread_mutex_t s_mutex;
	phtread_cond_t s_cond;
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
	thread_t **threads;					//pointer to threads
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
	thread_pool_t *tpool;				//pointer to thread pool
}thread_t;



#endif
