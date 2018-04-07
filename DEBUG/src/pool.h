/** $pool_h$ **/
/**
 * @file pool.h
 * @section LICENSE
 * ****************************************************************************
 * @author Luca Canessa (516639)   			                                  *
 *  																		  *
 *  \copyright \n                                                             *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * @section DESCRIPTION
 * Looking overview
 * 
 * This file contains prototypes of functions and data structures that 
 * manipulate and manage a thread pool.
 * The thread pool is a structure in which some worker threads are present to
 * perform tasks that are placed in a queue also present in this structure. To
 * perform tasks, each thread has assigned a worker who extrapolates a job from
 * the queue if present, otherwise it waits until someone inserts it. To create
 * a pool you need to use 'pool_creation ()', to add jobs to the pool 
 * 'threadpool_add()' and to delete a pool you need to use 
 * 'threadpool_destroy ()'
 * 
 * pool_creation()	:	Allocates space for a new thread pool then returns 
 * 						pointer to new pool.
 * 						Requires nothing
 * 						Returns the pointer to the new thread pool
 * 
 * threadpool_add()	:	Adds a new job to the queue to be executed
 * 						Requires a pointer to the thread pool to add the task 
 * 						and the function pointer and the related arguments to 
 * 						add to the queue
 * 						Returns 0 if exits without errors, otherwise -1
 * 
 * threadpool_destroy():Stops all threads worker according to the mode sent, it
 * 						can delete threads after they finished theri job or 
 * 						delete immediately.
 * 						Requires pointer to the thread pool
 * 						Returns 0 if terminate, else -1
 * 
*/



/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#ifndef _POOL_H_
#define _POOL_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>

#include <src/config.h>
#include <src/queue.h>



/******************************************************************************
								TYPE & STRUCTURES
******************************************************************************/
extern int _MAX_CONN;	///< max_conn is number of max connection to handle
extern int _THREADn;	///< num_thread is number of thread that operates in threadpool_t




typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;

typedef enum {
    threadpool_graceful       = 1
} threadpool_destroy_flags_t;



/**
 * @typedef			thread_task_t
 * @struct			struct task
 * @brief    		Defines a new type of 'thread_task_t' structure to 
 * 					implement the work to be run
 * @var function  	Pointer to function to be given to the threadpool worker
 * @var args 		Pointer to args of function
 */
typedef struct task
{
	void(*function)(void *);
	void *args;
}thread_task_t;



/**
 * @typedef			threadpool_t
 * @struct			struct pool
 * @brief    		Defines a new structure type 'threadpool_t' to implements 
 * 					the thread pool
 * @var thread  	Pointer to 'pthread_t' type items
 * @var task 		Pointer to queue of tasks to execute
 * @var lock_t		Variable to control the mutual exclusion
 * @var cond_t		Variable to synchronize threads in the thread pool
 * @var count		Variable to know the number of tasks in queue
 * @var shutdown	Variable to stop thread worker: 
 * 					if 1, kills threads after terminate their jobs, if 0 kills 
 * 					immediately all threads
 */
typedef struct pool
{
	pthread_t *thread;
	queue_t *task;
	pthread_mutex_t lock_t;
	pthread_cond_t cond_t;
	int count;
	int shutdown;
}threadpool_t;



/******************************************************************************
									FUNCTIONS
******************************************************************************/
/**
 * @brief		function to run task passed
 * @var	pool	pointer to thread pool where are tasks to extract
 */
// void thread_work( threadpool_t *pool );



/**
 * @function			pool_creation
 * @brief		Creates and initializes a thread pool
 * @return 		Pointer to thread pool created or NULL
 */
threadpool_t *pool_creation( );



/**
 * @function				 threadpool_add
 * @brief			 Adds task at thread pool task queue
 * @param	pool	 Pointer to 'threadpool_t' thread pool where add the task
 * @param	function Pointer to the function to add to the queue
 * @param	args	 Pointer to the args of job to add to queue
 * @return	0 	if terminates without error
 * 			-1	otherwise
 */
int threadpool_add( threadpool_t *pool, void(*function)(void *), void *args );



/**
 * @function					threadpool_destroy
 * @brief				Function to stop worker thread and destroy all pool
 * @param	pool		Pointer to the pool to destroy
 * @param	power_off	Flag to stop worker if 0 stops immediately, otherwise
 * 					 	it stops when there are no more jobs
 * @return				0 if terminates without errors, else -1
 */
int threadpool_destroy(threadpool_t *pool, int flags);



#endif
