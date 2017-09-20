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


extern int max_conn, num_thread;

typedef enum{
	false,
	true
}bool;

/******************************************************************************
									STRUCTURES
******************************************************************************/

typedef enum {
    threadpool_graceful       = 1
} threadpool_destroy_flags_t;


typedef struct task
{
	void(*function)(void *);
	void *args;
}thread_task_t;

typedef struct pool
{
	pthread_t *thread;
	thread_task_t *task;
	pthread_mutex_t lock_t;
	pthread_cond_t cond_t;
	int thread_crt;
	int queue_size;
	int head;
	int tail;
	int count;
	int shutdown;
	int started;
}threadpool_t;


/******************************************************************************
									FUNCTIONS
******************************************************************************/

void thread_work( void *args );


void thread_stop( threadpool_t *pool );


int threadpool_add( threadpool_t *pool, void(*function)(void *), void *args );

int threadpool_free(threadpool_t *pool);

int threadpool_destroy(threadpool_t *pool, int flags);

void print( void *arg );


threadpool_t *pool_creation( );

#endif
