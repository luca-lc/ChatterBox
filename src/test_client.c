/*
 * test_client.c
 *
 *  Created on: Dec 11, 2017
 *      Author: luca
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>




#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <src/connections.h>

#define N 10



int main(void)
{
	char sock[] = "/home/luca/workspace/ChatterBox/tmp/chatty_socket";
	int c_fd = openConnection( sock, 3, 2 );

	message_t msg;
	char tmp[512] = "/home/luca/Sites/diricons.png";
	setHeader( &msg.hdr, REGISTER_OP, "Luca" );
	setData( &msg.data, "", NULL, 0 );


	sendRequest( c_fd, &msg );

	readHeader( c_fd, &msg.hdr );

	if( msg.hdr.op == OP_OK )
	{
		printf( "User %s registered\n" );
	}
	else
	{
		printf( "User %s not been registered\n" );
	}

	setHeader( &msg.hdr, DISCONNECT_OP, "Luca" );
	setData( &msg.data, "", NULL, 0 );
	sendRequest( c_fd, &msg );

//	struct stat st;
//	if (stat(msg.data.buf, &st)==-1)
//	{
//		printf ("file does not exist" );
//		return -1;
//	}
//	if (!S_ISREG(st.st_mode))
//	{
//		printf ("file does not exist" );
//		return -1;
//	}
//
//	int ffd = open(tmp, O_RDONLY );
//	if( ffd < 0 )
//		return -1;
//
//
//	if (sendRequest(c_fd, &msg) == -1)
//		return -1;
//
//
//	char *mappedfile = mmap(NULL, st.st_size, PROT_READ,MAP_PRIVATE, ffd, 0);
//	message_data_t data;
//
//	if( mappedfile )
//	{
//		setData(&data, "", mappedfile, st.st_size );
//
//		if( sendData( c_fd, &data ) == -1 )
//			return -1;
//		else
//			printf( "msg sent\n" );
//	}
//
//
//
//
//

//	printf("Client %d ha inviato il messaggio\n", getpid());

	sleep( 5 );

	close( c_fd );

	return 0;
}
