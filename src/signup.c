/* $pool.c$ */

/**
 * @file signup.c
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c)2017 Luca Canessa (516639)                                    *
 *                                                                            *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 * @section DESCRIPTION
 * Looking overview
 *
*/


/******************************************************************************
									HEADER
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <queue.h>
#include <ops.h>
#include <stats.h>
#include <signup.h>


static pthread_mutex_t reg_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t reg_cond = PTHREAD_COND_INITIALIZER;



/******************************************************************************
									FUNCTIONS
******************************************************************************/

