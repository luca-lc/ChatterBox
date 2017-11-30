/* $signup.h$ */
/**
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c)2017 Luca Canessa (516639)                                    *
 *                                                                            *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
*/


/******************************************************************************
									HEADER
******************************************************************************/
#ifndef SIGNUP_H_
#define SIGNUP_H_

#include <src/ops.h>
#include <src/hashtable.h>
#include "src/chatty.h"




typedef struct args
{
	hashtable_t *myt;
	char *name;
}checkin_arg;

/******************************************************************************
									FUNCTIONS
******************************************************************************/
/**
 *
 */
bool checkin( checkin_arg *c_arg );



/**
 *
 */
bool delete( checkin_arg *c_arg );

#endif /* SIGN_UP_H_ */
