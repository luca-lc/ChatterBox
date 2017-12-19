/* $connections.c$ */
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <src/connections.h>



/******************************************************************************
									FUNTIONS
******************************************************************************/
/**
 *
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs)
{
	int sockfd;
	struct sockaddr_un serv_addr;

	sockfd = socket( AF_UNIX, SOCK_STREAM, 0 );
	memset( &serv_addr, '0', sizeof( serv_addr ) );

	serv_addr.sun_family = AF_UNIX;
	strncpy( serv_addr.sun_path, path, strlen( path )+1 );

	
	int mfd;
	while( (mfd = connect( sockfd, (struct sockaddr*)&serv_addr, sizeof( serv_addr ) )) != 0 && ntimes != 0 )
	{
		ntimes--;
		sleep( secs );
	}

	if( ntimes == 0 )
	{
		return -1;
	}
	else
	{
		return sockfd;
	}
}



/**
 * @brief
 */
int readHeader(long connfd, message_hdr_t *hdr)
{
	int r;
	char *buffptr = (char *)hdr;
	size_t left = sizeof(hdr);
	while( left > 0 )
	{
		if( (r = read( (int)connfd, buffptr, left)) == -1)
		{
			if( errno == EINTR )
				continue;
			return -1;
		}
		if( r == 0 )
			return 0;
		left -= r;
	}
	return( sizeof(hdr) );
}



/**
 * @brief
 */
int sendRequest(long fd, message_t *msg)
{
	size_t left = sizeof( msg->hdr );
	int r;
	char *buffptr = (char *)&msg->hdr;
	while(left>0)
	{
		if( (r = write((int)fd, buffptr,left)) == -1 )
		{
			if( errno == EINTR )
				continue;
			return -1;
		}
		if( r == 0 )
			return 0;
		left -= r;
	}

	return( sizeof( msg->hdr ) );
}



/**
 * @brief
 */
int readData(long fd, message_data_t *data)
{
	int r;
		char *buffptr = (char *)data;
		size_t left = sizeof(data);
		while( left > 0 )
		{
			if( (r = read( (int)fd, buffptr, left)) == -1)
			{
				if( errno == EINTR )
					continue;
				return -1;
			}
			if( r == 0 )
				return 0;
			left -= r;
		}
		return( sizeof(data) );

//	left = sizeof( data->buf );
//	buffptr = (char *)&data->buf;
//	while(left>0)
//	{
//		if( (r = read((int)fd, buffptr,left)) == -1 )
//		{
//			if( errno == EINTR )
//				continue;
//			return -1;
//		}
//		if( r == 0 )
//			return 0;
//		left -= r;
//	}

//	return( sizeof( data ) );
}



/**
 * @brief
 */
int readMsg(long fd, message_t *msg)
{
//	size_t left = sizeof( msg->data.buf );
//	int r;
//	char *buffptr = (char *)&msg->data.buf;
//	while(left>0)
//	{
//		if( (r = read((int)fd, buffptr,left)) == -1 )
//		{
//			if( errno == EINTR )
//				continue;
//			return -1;
//		}
//		if( r == 0 )
//			return 0;
//		left -= r;
//	}
//
//	return( sizeof( msg->data.buf ) );
	return 0;
}



/**
 * @brief
 */
int sendData(long fd, message_data_t *msg)
{
	size_t left = sizeof( msg->buf );
	int r;
	char *buffptr = (char *)&msg->buf;
	while(left>0)
	{
		if( (r = write((int)fd, buffptr,left)) == -1 )
		{
			if( errno == EINTR )
				continue;
			return -1;
		}
		if( r == 0 )
			return 0;
		left -= r;
	}

	return( sizeof( msg ) );
}

