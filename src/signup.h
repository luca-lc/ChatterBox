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


/******************************************************************************
									FUNCTIONS
******************************************************************************/
/**
 *
 */
int checkin( hashtable_t *users, char *nick );



/**
 *
 */
user_t *connecting( hashtable_t *users, char *nick );



/**
 *
 */
bool delete( hashtable_t *users, char *nick );

#endif /* SIGN_UP_H_ */
