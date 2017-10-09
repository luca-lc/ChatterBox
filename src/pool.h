/* $pool.h$ */
/**
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c)2017 Luca Canessa (516639)                                    *
 *                                                                            *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
*/



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



/******************************************************************************
								TYPE & STRUCTURES
******************************************************************************/
extern int max_conn, num_thread;	///< max_conn is number of max connection to handle
									///< num_thread is number of thread that operates in threadpool_t



typedef enum {
    threadpool_graceful       = 1
} threadpool_destroy_flags_t;



/**
 * @brief    		defines a new structure type 'thread_task_t' to implements task to do
 * @var function  	pointer to function to be given to the threadpool worker
 * @var args 		pointer to args for function
 * @var next		val to find the last added function in threadpool
 */
typedef struct task
{
	void(*function)(void *);
	void *args;
	int next;
}thread_task_t;



/**
 * @brief    		defines a new structure type 'threadpool_t' to implements the threadpool
 * @var thread  	pointer to function to be given to the threadpool worker
 * @var task 		pointer to queue of tasks to do
 * @var lock_t		var to lock pool with mutex
 * @var cond_t		condition variable to block pool
 * @var thread_crt	counter to know number of thread at work
 * @var queue_size	var to know the queue length of tasks
 * @var count		var to know the number of tasks entered
 * @var next_max	var to know the last task entered
 * @var shutdown	var to stop thread worker: if 1 waits to end all works else stops immediately
 */
typedef struct pool
{
	pthread_t *thread;
	thread_task_t *task;
	pthread_mutex_t lock_t;
	pthread_cond_t cond_t;
	int thread_crt;
	int queue_size;
	int count;
	int shutdown;
	int next_max;
}threadpool_t;



typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;



/******************************************************************************
									FUNCTIONS
******************************************************************************/
/**
 * @brief		function to run task passed
 * @var	pool	pointer to thread pool where are tasks to extract
 */
void thread_work( threadpool_t *pool );



/**
 * @brief		function to creates and initializes a thread pool
 * @return 		pointer to thread pool created and initialized
 */
threadpool_t *pool_creation( );



/**
 * @brief		function to add task at thread pool task queue
 * @var	pool	pointer to threadpool_t where add the task
 * @var	args	pointer to args of task to add to queue
 * @return	0 	if terminates without error
 * 			-1	otherwise
 */
int threadpool_add( threadpool_t *pool, void(*function)(void *), void *args );



/**
 * @brief			function to stop worker thread immediately or after finished their jobs and destroy all pool
 * @var	pool		pointer to pool to destroy
 * @var	power_off	flag to stop worker, if 0 then stop immediately else stop when there are no more works
 * @return			0 if terminates without errors, else -1
 */
int threadpool_destroy(threadpool_t *pool, int flags);



/**
 * @brief		function to stop immediately all threads
 * @var	pool	pointer to the thread pool where is var to control shutting down
 */
void thread_stop( threadpool_t *pool );



#endif
