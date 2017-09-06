#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>


typedef struct args
{
	int tid;
}thread_args;


extern int num_pool = 5;
