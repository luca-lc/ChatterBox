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


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <src/connections.h>

#define N 10

int main(void)
{
	char sock[] = "/home/luca/eclipse-workspace/ChatterBox/tmp/chatty_socket";
	int c_fd = openConnection( sock, 3, 2 );

	if( c_fd > -1 )
	{
		message_t msg;
		char tmp[50] = "ciao\040 come\040 stai?\0";
		setHeader( &msg.hdr, DELGROUP_OP, "lucone_1995" );
		setData( &msg.data, "Roby", tmp, strlen(tmp)+1 );
		if (sendRequest(c_fd, &msg) == -1)
			return -1;
		else
			printf( "header sent\n" );
		printf( "%d\n", strlen(tmp) + 1);
		if( sendData( c_fd, &msg.data ) == -1 )
			return -1;
		else
			printf( "msg sent\n" );



		printf("Client %d ha inviato il messaggio\n", getpid());

		sleep( 2 );
		close( c_fd );
	}
	else
	{
		printf( "impossible to connect to server\n" );
	}

	return 0;
}
