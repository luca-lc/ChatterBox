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
	size_t size_buf = sizeof(hdr->op) + sizeof(hdr->sender);
	char *buff;

	if( (buff = ( char * )malloc( size_buf * sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocate space" );
		return EXIT_FAILURE;
	}


	int left, r;

	//receive buffer
	left = size_buf, r = 0;
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

	memcpy( &hdr->op, buff, sizeof( int ) );//copy op in hdr's field
	memcpy( hdr->sender, buff+sizeof( int), sizeof( hdr->sender ) ); //copy sender in hdr's field

	free( buff );

	return( size_buf );
}



/**
 * @brief
 */
int sendRequest(long fd, message_t *msg)
{
	size_t size_buf = sizeof(msg->hdr.op) + sizeof(msg->hdr.sender) + sizeof(msg->data.hdr.len) + sizeof(msg->data.hdr.receiver) + msg->data.hdr.len + 1;
	int left, r;
	char *buff;
	if( (buff = ( char * )malloc( size_buf * sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocate space" );
		return EXIT_FAILURE;
	}

	int offset = 0;
	memcpy( buff + offset, &msg->hdr.op, sizeof( int ) ); //copy op in buffer

	offset = sizeof( int );
	memcpy( buff+offset, &msg->hdr.sender, sizeof( msg->hdr.sender ) ); //copy sender in buffer.

	offset = sizeof( int ) + sizeof( msg->hdr.sender );
	memcpy( buff+offset, &msg->data.hdr.len, sizeof(msg->data.hdr.len) ); //copy body len in buffer

	offset = sizeof( int ) + sizeof( msg->hdr.sender ) + sizeof(msg->data.hdr.len);
	memcpy( buff+offset, &msg->data.hdr.receiver, sizeof(msg->data.hdr.receiver) ); //copy receiver name in buffer

	offset = sizeof( int ) + sizeof( msg->hdr.sender ) + sizeof(msg->data.hdr.len) + sizeof(msg->data.hdr.receiver);
	memcpy( buff+offset, &msg->data.buf, msg->data.hdr.len ); //copy body msg in buffer

	buff[size_buf-1] = '\0';


	//send buffer
	left = size_buf, r = 0;
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

	free( buff );

	return( size_buf + msg->data.hdr.len );
}



/**
 * @brief
 */
int readData(long fd, message_data_t *data)
{
	int byte_buf = 1024*1024;
	size_t size_buf = sizeof(data->hdr.len) + sizeof(data->hdr.receiver) + 5;
	char *buff = NULL;
	if( (buff = ( char * )malloc(size_buf * sizeof( char ))) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocate space" );
		return EXIT_FAILURE;
	}
	if ( (data->buf = ( char * )malloc( data->hdr.len * sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocate space" );
		return EXIT_FAILURE;
	}


	//ricevo buffer
	int left = size_buf, r = 0;
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

	//divido contenuti
	memcpy( &data->hdr.len, buff, sizeof( int ) );
    memcpy( data->hdr.receiver, buff+sizeof( unsigned int ), sizeof( data->hdr.receiver ) );

    //ricevo body
	left = data->hdr.len, r = 0;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, data->buf, left, 0)) == -1 )
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

	free( buff );
	return( size_buf + data->hdr.len );
}



/**
 * @brief
 */
int readMsg(long fd, message_t *msg)
{
	int left = 0, r = 0;
	size_t size_buf = sizeof(msg->hdr.op) + sizeof(msg->hdr.sender) + sizeof(msg->data.hdr.len) + sizeof(msg->data.hdr.receiver) + 1;
	char *buff = NULL;
	if( (buff = ( char * )malloc(size_buf * sizeof( char ))) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocate space" );
		return EXIT_FAILURE;
	}

	//ricevo buffer
	left = size_buf, r = 0;
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

	int offset = 0;
	memcpy( &msg->hdr.op, buff + offset, sizeof( int ) ); //copy op in buffer

	offset = sizeof( int );
	memcpy( &msg->hdr.sender, buff+offset, sizeof( msg->hdr.sender ) ); //copy sender in buffer.

	offset = sizeof( int ) + sizeof( msg->hdr.sender );
	memcpy( &msg->data.hdr.len, buff+offset, sizeof(msg->data.hdr.len) ); //copy body len in buffer

	offset = sizeof( int ) + sizeof( msg->hdr.sender ) + sizeof(msg->data.hdr.len);
	memcpy( &msg->data.hdr.receiver, buff+offset, sizeof(msg->data.hdr.receiver) ); //copy receiver name in buffer

	free( buff );


	//ricevo body

	left = msg->data.hdr.len, r = 0;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, msg->data.buf, left, 0)) == -1 )
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




	return 0;
}



/**
 * @brief
 */
int sendData( long fd, message_data_t *msg )
{
	size_t size_buf = sizeof(msg->hdr.len) + sizeof(msg->hdr.receiver) + 5;
	char *buff = NULL;
	if( (buff = ( char * )malloc(size_buf * sizeof( char ))) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocate space" );
		return EXIT_FAILURE;
	}

	//copio elementi nel buffer
	memcpy( buff, (char *)&msg->hdr.len, sizeof( int ) );
	memcpy( buff+sizeof( unsigned int ), msg->hdr.receiver, size_buf - sizeof( unsigned int ) );

	//invio buffer
	int left = size_buf, r = 0;
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

	sleep( 0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001 );
	left = msg->hdr.len, r = 0;

	while( left > 0 )
	{
		if( (r = send( (int)fd, msg->buf, left, 0)) == -1 )
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

	free( buff );
	return( size_buf + msg->hdr.len );
}
