/*
 * prova.h
 *
 *  Created on: Oct 9, 2017
 *      Author: luca
 */

#ifndef SRC_PROVA_H_
#define SRC_PROVA_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void print( )
{
	char *c = (char *)malloc( 10 *sizeof(char));
	printf( "insert\n" );
	gets( c );
	printf( "printed: \t%s\n", c );
}



#endif /* SRC_PROVA_H_ */
