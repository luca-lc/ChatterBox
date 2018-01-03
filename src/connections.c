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
	char *buff = NULL;
	char tmp_op[20];
	int left, r;

	//read operation
	sprintf( tmp_op, "%d", hdr->op );
	left = sizeof( hdr->op ), r = 0;
	while( left > 0 )
	{
		if( (r = recv( (int)connfd, tmp_op, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}


	//read sender
	left = sizeof( hdr->sender )-1, r = 0;
	buff = (char *)hdr->sender;
	while( left > 0 )
	{
		if( (r = recv( (int)connfd, buff, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}
//	hdr->sender[sizeof( hdr->sender)] = '\0';
	return( sizeof(hdr->op) + sizeof(hdr->sender) );
}



/**
 * @brief
 */
int sendRequest(long fd, message_t *msg)
{
	char *buff = NULL;
	char tmp_op[20];
	int left, r;

	//send operation
	sprintf( tmp_op, "%d", msg->hdr.op );
	left = sizeof( msg->hdr.op ), r = 0;
	while( left > 0 )
	{
		if( (r = send( (int)fd, tmp_op, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}


	//send sender
	left = sizeof( msg->hdr.sender )-1, r = 0;
	buff = (char *)msg->hdr.sender;
	while( left > 0 )
	{
		if( (r = send( (int)fd, buff, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}
//	msg->hdr.sender[sizeof( msg->hdr.sender)] = '\0';

	return( sizeof(msg->hdr.op) + sizeof(msg->hdr.sender) );
}



/**
 * @brief
 */
int readData(long fd, message_data_t *data)
{
	char *buff = NULL;
	char tmp_len[20];
	int left, r;

	//length reception
	memset( &tmp_len[0], '0', sizeof( tmp_len ) );
	left = sizeof( data->hdr.len ), r = 0;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, tmp_len, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}
	data->hdr.len = atoi( tmp_len );

	//receiver reception
	left = sizeof( data->hdr.receiver ), r = 0;
	buff = (char *)data->hdr.receiver;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, buff, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}


	//receiver message body
	data->buf = (char *)malloc( data->hdr.len+1 * sizeof( char ) );
	left = data->hdr.len, r = 0;
	buff = (char *)data->buf;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, buff, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}
	data->buf[data->hdr.len] = '\0';

	return( data->hdr.len + sizeof(data->hdr.receiver) + sizeof(data->buf) );
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
int sendData( long fd, message_data_t *msg )
{
	char *buff = NULL;
	char tmp_len[20];
	int left, r;

	//send length
	sprintf( tmp_len, "%d", msg->hdr.len );
	left = sizeof( msg->hdr.len ), r = 0;
	while( left > 0 )
	{
		if( (r = send( (int)fd, tmp_len, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}


	//send receiver
	left = sizeof( msg->hdr.receiver ), r = 0;
	buff = (char *)msg->hdr.receiver;
	while( left > 0 )
	{
		if( (r = send( (int)fd, buff, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}
	msg->hdr.receiver[sizeof( msg->hdr.receiver)] = '\0';

	//send message body
	left = msg->hdr.len+1, r = 0;
	buff = (char *)msg->buf;
	while( left > 0 )
	{
		if( (r = send( (int)fd, buff, left, 0)) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		left -= r;
	}
	msg->buf[msg->hdr.len] = '\0';


	return( msg->hdr.len + sizeof(msg->hdr.receiver) + sizeof(msg->buf) );
}
