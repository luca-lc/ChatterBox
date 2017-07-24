//
// CHATTERBOX_SIGN_UP_H
// Created by Luca Canessa - mat. 516639.
// Declares that all contents of this file are author's original operas
//


/******************************************************************************
									HEADER
******************************************************************************/
#ifndef SIGN_UP_H_
#define SIGN_UP_H_

#include <ops.h>

typedef struct usr
{
	char nikname[512];
}user_t;

/******************************************************************************
									FUNCTIONS
******************************************************************************/

/**
 * @brief		checks if a element (user) is in the queue 'reg'
 * @param reg	pointer to queue of registered users
 * @param elem	pointer to user to check if is in the queue
 * @return 		op_t type value
 */
//op_t is_registered( queue_t *reg, void *elem );



/**
 * @brief 		checks if the user 'user' is registered, if not record him
 * @param user	pointer to user to record
 * @param reg	pointer to queue of registered users
 */
void sign_up( void *user, queue_t *reg );



#endif /* SIGN_UP_H_ */
