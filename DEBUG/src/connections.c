/** $connections_c$ **/

/**
 * @file connections.c
 * @section LICENSE
 * ****************************************************************************
 * @author Luca Canessa (516639)                                    		  *
 *                                                                            *
 * @copyright \n															  *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 *
 * @section DESCRIPTION
 * Looking overview
 * 
 * In this file are present functions to manage connections from and to 
 * clients. Every communication between server and clients is handled using 
 * messages. Each request from clients is presents in the message header and 
 * into the message body, there are the arguments to manage the request. Each 
 * received datum is in char representation so each function that manages this,
 * it must cast in datum type. Instead, if the datum shall be sent, the 
 * function that manges it, must save the datum in char into a buffer.
 * 
 * openConnection()	:	Its task is to open a file descriptor for a socket.
 * 						Requires a pointer to the path where save the socket 
 * 						that will be open, the number of time to retry to 
 * 						connect if there are some problems and the seconds that
 * 						must pass between the retrying.
 * 						Returns the idetifier of file destriptor.
 * 
 * readHeader()		:	Receives a buffer where are present the message header 
 * 						items.
 * 						Requires the identifier of file descriptor and pointer
 * 						to a message header where save the data received.
 * 						Returns the number of bytes read or -1 if exits with 
 * 						some errors.
 * 
 * sendHeader()		:	Sends a buffer where are present the message header 
 * 						items. 
 * 						Requires the identifier of file descriptor and the 
 * 						pointer to a message header where saved the data to be 
 * 						sent. 
 * 						Returns the number of bytes sent or -1 if exits with 
 * 						some errors.
 * 
 * sendRequest()	:	Sends a buffer where are present the message items. 
 * 						Requires the identifier of file descriptor and the 
 * 						pointer to a message where saved the data that will be 
 * 						managed by server.
 * 						Returns the number of bytes sent or -1 if exits with 
 * 						some errors.
 * 
 * readData()		:	Receives a buffer where are present the message body 
 * 						items.
 * 						Requires the identifier of file descriptor and pointer
 * 						to a message body where save the data received.
 * 						Returns the number of bytes read or -1 if exits with 
 * 						some errors.
 * 
 * sendData()		:	Sends a buffer where are present the message body items 
 * 						Requires the identifier of file descriptor and the 
 * 						pointer to a message body where saved the data to be 
 * 						sent. 
 * 						Returns the number of bytes sent or -1 if exits with 
 * 						some errors.
 * 
 * readMsg()		:	Receives a buffer where are present the message items. 
 * 						Requires the identifier of file descriptor and the 
 * 						pointer to a message where save the data that will be 
 * 						managed by server.
 * 						Returns the number of bytes read or -1 if exits with 
 * 						some errors.
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
#include <src/ops.h>

int _FILESIZE, _MSGSIZE;    ///< _FILESIZE is the max size of a file could be sent - _MSGSIZE is the max size of a message could be sent
char SOCKNAME[UNIX_PATH_MAX]; ///< path where is saved the socket
char Dir[256];			///< path where save the files received



/******************************************************************************
									FUNTIONS
******************************************************************************/
/**
 * @function		openConnection
 * @brief 			opens a connection from client to server
 * @param	path	path where located the socket
 * @param	ntimes	number of reconnection attempts
 * @param 	secs	seconds that elapse between attempts
 * @return	sockfd	identifier of file descriptor
 */
int openConnection( char* path, unsigned int ntimes, unsigned int secs )
{
	int sockfd;
	struct sockaddr_un serv_addr;

	sockfd = socket( AF_UNIX, SOCK_STREAM, 0 );

	memset( &serv_addr, '0', sizeof( serv_addr ) );
	serv_addr.sun_family = AF_UNIX;
	strncpy( serv_addr.sun_path, path, strlen( path )+1 );


	while( (connect( sockfd, (struct sockaddr*)&serv_addr, sizeof( serv_addr )) != 0) && (ntimes != 0)	 )
	{

		printf( "retry to connect again %d time(s)\n", ntimes );
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
 * @function		readHeader
 * @brief			Receives a buffer where are all header items to know a 
 * 					request or to know the server response.
 * 					Save the items in a buffer and then copy them one at a time
 * 					in the passed header.
 * @param	connfd	Identifier of file  descriptor
 * @param	hdr		Pointer to the header of the message in which the received 
 * 					items will be saved
 * @return  size_buf Number of byte read or -1 if exits with error
 */
int readHeader(long connfd, message_hdr_t *hdr)
{
	size_t size_buf = sizeof(hdr->op) + sizeof(hdr->sender);
	char *buff = NULL;

	if( (buff = ( char * )calloc( size_buf, sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocating space for buffer" );
		return EXIT_FAILURE;
	}

	int left = size_buf, r = 0, s = 0;

	//receive the buffer
	while( left > 0 )
	{
		if( (r = recv( (int)connfd, buff+s, left, 0)) == -1 )
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
		s += r;
		left -= r;
	}

	//divide the buffer contents
	int offset = 0;
	memcpy( &hdr->op, buff + offset, sizeof( hdr->op ) ); //copy op in hdr's field

	offset += sizeof( hdr->op );
	memcpy( hdr->sender, buff + offset, sizeof( hdr->sender ) ); //copy sender in hdr's field

	if( buff )
		{
			free( buff );
		}

	return( size_buf );
}



/**
 * @function		sendHeader
 * @brief			Sends a buffer where are all header items to respond to the client.
 * 					Copy the items one at a time from the passed header to a buffer and sends it.
 * @param	connfd	Identifier of file  descriptor
 * @param	hdr		Pointer to the header of the message in which the received 
 * 					items will be saved
 * @return  size_buf Number of byte sent or -1 if exits with error
 */
int sendHeader(long connfd, message_hdr_t *hdr)
{
	size_t size_buf = sizeof(hdr->op) + sizeof(hdr->sender);
	char *buff = NULL;

	if( (buff = ( char * )calloc( size_buf, sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocating space for buffer" );
		return EXIT_FAILURE;
	}

	//divide the buffer contents
	int offset = 0;
	memcpy( buff + offset, &hdr->op, sizeof( hdr->op ) ); //copy op in hdr's field

	offset += sizeof( hdr->op );
	memcpy( buff + offset, hdr->sender, sizeof( hdr->sender ) ); //copy sender in hdr's field

	int left = size_buf, r = 0, s = 0;

	//send the buffer
	while( left > 0 )
	{
		if( (r = send( (int)connfd, buff+s, left, 0)) == -1 )
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
		s += r;
		left -= r;
	}
	
	if( buff )
		{
			free( buff );
		}

	return( size_buf );
}



/**
 * @function		sendRequest
 * @brief			Send a request to the server. This request is a message 
 * 					with: code to define the request in the header and, if 
 * 					necessary, the other parts of the message to handle the 
 * 					request. Copies all items in a buffer and send it.
 * 					The copy and send are divided in three blocks:
 * 					first, copies all items and send the header of the message
 * 					second, sends the header of body of the message
 * 					third, send directly the data of the body of the message.
 * @param	connfd	Identifier of file  descriptor
 * @param	msg		Pointer to the message where located the request
 * @return  size_buf Number of byte sent or -1 if exits with error
 */
int sendRequest(long fd, message_t *msg)
{
	size_t size_buf = sizeof(msg->hdr.op) + sizeof( msg->hdr.sender) + sizeof(msg->data.hdr.len) + sizeof(msg->data.hdr.receiver) + msg->data.hdr.len;
	int left, r, s;
	char *buff = NULL;;


	if( (buff = ( char * )calloc( size_buf, sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocate space" );
		return EXIT_FAILURE;
	}

	//copy message elements in the buffer
	int offset = 0;
	memcpy( buff+offset, &msg->hdr.op, sizeof( int ) ); //copy op in buffer

	offset += sizeof( int );
	memcpy( buff+offset, msg->hdr.sender, sizeof( msg->hdr.sender ) ); //copy sender in buffer.

	offset += sizeof( msg->hdr.sender );
	memcpy( buff+offset, &msg->data.hdr.len, sizeof(msg->data.hdr.len) ); //copy body len in buffer

	offset += sizeof(msg->data.hdr.len);
	memcpy( buff+offset, msg->data.hdr.receiver, sizeof(msg->data.hdr.receiver) ); //copy receiver name in buffer

	offset += sizeof(msg->data.hdr.receiver);
	memcpy( buff+offset, msg->data.buf, msg->data.hdr.len ); //copy body msg in buffer

	//send header
	left =  sizeof(msg->hdr.op) + sizeof( msg->hdr.sender ), r = 0, s = 0;
	while( left > 0 )
	{
		if( (r = send( (int)fd, buff+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		s += r;
		left -= r;
	}

	//send body header
	left =  sizeof(msg->data.hdr.len) + sizeof(msg->data.hdr.receiver), r = 0;
	while( left > 0 )
	{
		if( (r = send( (int)fd, buff+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		s += r;
		left -= r;
	}

	//send body
	left =  msg->data.hdr.len, r = 0;
	while( left > 0 )
	{
		if( (r = send( (int)fd, buff+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		s += r;
		left -= r;
	}

	if( buff )
		{
			free( buff );
		}
		
	return( size_buf );
}



/**
 * @function		readData
 * @brief			Receives a buffer where are all body items to manage a 
 * 					request or to receive data from server
 * 					Saves the items in a buffer and then copy them one at a time
 * 					in the passed header. The receive is dived in two:
 * 					first, receives the message body header
 * 					second, receives the data of the message
 * @param	connfd	Identifier of file  descriptor
 * @param	data	Pointer to the body of the message in which the received 
 * 					items will be saved
 * @return  size_buf Number of byte read or -1 if exits with error
 */
int readData(long fd, message_data_t *data)
{
	size_t size_buf = sizeof(data->hdr.len) + sizeof(data->hdr.receiver);
	char *buff = NULL;


	if( (buff = ( char * )calloc( size_buf, sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocating space for buffer" );
		return EXIT_FAILURE;
	}

	//receive the buffer
	int left = size_buf, r = 0, s = 0;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, buff+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			break;
		}
		s += r;
		left -= r;
	}

	//divide the buffer contents
	int offset = 0;
	memcpy( &data->hdr.len, buff + offset, sizeof( data->hdr.len ) );
	offset += sizeof( int );
    memcpy( data->hdr.receiver, buff + offset, sizeof( data->hdr.receiver ) );

	data->buf = (char *)calloc( data->hdr.len, sizeof(char) );
	left = data->hdr.len, r = 0, s = 0;

	//receive body
	while( left > 0 )
	{
		if( (r = recv( (int)fd, data->buf+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			break;
		}
		s += r;
		left -= r;
	}

	if( buff )
		{
			free( buff );
		}
	return( size_buf + data->hdr.len );
}



/**
 * @function		sendData
 * @brief			Sends a buffer where are all body items to manage a request
 *  				or to send data from server. Saves the items in a buffer 
 * 					coping them one at a time in the passed message body. The 
 * 					receive is dived in two:
 * 					first, receives the message body header
 * 					second, receives the data of the message
 * @param	connfd	Identifier of file  descriptor
 * @param	msg		Pointer to the body of the message in which saved the items
 * @return  size_buf Number of byte sent or -1 if exits with error
 */
int sendData( long fd, message_data_t *msg )
{
	size_t size_buf = sizeof(msg->hdr.len) + sizeof(msg->hdr.receiver);
	char *buff = NULL;
	if( (buff = ( char * )calloc( size_buf, sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocating space for buffer" );
		return EXIT_FAILURE;
	}

	//copy message fields in the buffer
	int offset = 0;
	memcpy( buff + offset, &msg->hdr.len, sizeof( msg->hdr.len ) ); //copy body len
	offset += sizeof( msg->hdr.len );
	memcpy( buff + offset, msg->hdr.receiver, sizeof( msg->hdr.receiver ) ); //copy receiver

	//send buffer
	int left = size_buf, r = 0, s = 0;
	while( left > 0 )
	{
		if( (r = send( (int)fd, buff+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			break;
		}
		s += r;
		left -= r;
	}

	//send body
	left = msg->hdr.len, r = 0, s = 0;
	while( left > 0 )
	{
		if( (r = send( (int)fd, msg->buf+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		s += r;
		left -= r;
	}

	if( buff )
		{
			free( buff );
		}
	return( size_buf + msg->hdr.len );
}




/**
 * @function		readMsg
 * @brief			Receives a request to the server. This request is a message 
 * 					with: code to define the request in the header and the 
 * 					other parts of the message to handle the request. Receives 
 * 					all items in a buffer and copies the items into the 
 * 					appropriate camp of the message passed. The receive and 
 * 					copy are divided in three blocks:
 * 					first, receives header items and copies them into the 
 * 					header of the message
 * 					second, receives and then copies the message body header
 * 					into the message passed
 * 					third, receives directly the data of the body
 * @param	connfd	Identifier of file  descriptor
 * @param	msg		Pointer to the message where located the request
 * @return  size_buf Number of byte received or -1 if exits with error
 */
int readMsg(long fd, message_t *msg)
{
	int left = 0, r = 0, s= 0;
	size_t size_buf = sizeof(msg->hdr.op) + sizeof(msg->hdr.sender) + sizeof(msg->data.hdr.len) + sizeof(msg->data.hdr.receiver);
	char *buff = NULL;
	if( (buff = ( char * )calloc( size_buf, sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocating space for buffer" );
		return EXIT_FAILURE;
	}

	//receive hdr
	left = sizeof(msg->hdr.op) + sizeof(msg->hdr.sender), r = 0, s = 0;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, buff+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		s += r;
		left -= r;
	}

	int offset = 0;
	memcpy( &msg->hdr.op, buff + offset, sizeof( int ) ); //copy op

	offset += sizeof( int );
	memcpy( msg->hdr.sender, buff+offset, sizeof( msg->hdr.sender ) ); //copy sender



	//receive body hdr
	left = sizeof(msg->data.hdr.len) + sizeof(msg->data.hdr.receiver), r = 0;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, buff+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		s += r;
		left -= r;
	}

	offset += sizeof( msg->hdr.sender );
	memcpy( &msg->data.hdr.len, buff+offset, sizeof( msg->data.hdr.len ) ); //copy body len

	offset += sizeof( msg->data.hdr.len );
	memcpy( msg->data.hdr.receiver, buff+offset, sizeof( msg->data.hdr.receiver ) ); //copy receiver name



	//space allocation for the message body
	if( (msg->data.buf = ( char * )calloc( msg->data.hdr.len, sizeof( char ) )) == NULL )
	{
		perror( "malloc()" );
		fprintf( stderr, "Problem to allocating space for message body" );
		return EXIT_FAILURE;
	}
	//receive body
	left = msg->data.hdr.len, r = 0, s = 0;
	while( left > 0 )
	{
		if( (r = recv( (int)fd, msg->data.buf+s, left, 0)) == -1 )
		{
			if( errno == EINTR || errno == EAGAIN )
			{
				continue;
			}
			return -1;
		}
		if( r == 0 )
		{
			return 0;
		}
		s += r;
		left -= r;
	}

	if( msg->data.hdr.len == 0 )
		{
			if( msg->data.buf )
				free( msg->data.buf );
			msg->data.buf = NULL;
		}

	if( buff )
		{
			free( buff );
		}

	return ( size_buf + msg->data.hdr.len );
}
