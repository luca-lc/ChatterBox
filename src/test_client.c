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

	int c_fd;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	c_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	while (connect(c_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1)
	{
		if (errno == ENOENT)
			sleep(1);
		else
		{
			perror("connect");
			exit(EXIT_FAILURE);
		}
	}

    message_t msg;
    setHeader( &msg.hdr, GETFILE_OP, "luca" );
    setData( &msg.data, "Roby", "ciao-mondo", 20 );
    if (sendRequest(c_fd, &msg) == -1)
    	return -1;
    else
    	printf( "header sent\n" );
    sleep( 2 );

    if( sendData( c_fd, &msg.data ) == -1 )
    	return -1;
    else
    	printf( "msg sent\n" );

    sleep( 1 );


//	char buff[20];
//	for( int i = 0; i < N-5; i++ )
//	{
//		sprintf( buff, "Hallo!\t%d", i );
//		write(c_fd, buff, 20);
//		sleep( 3 );
//		memset(&buff[0], 0, sizeof(buff));
//	}
//	memset(&buff[0], 0, sizeof(buff));
//	sprintf( buff, "close" );
//	write(c_fd, buff, 20);

	printf("Client %d ha inviato il messaggio\n", getpid());

	sleep(1);
	close( c_fd );

	return 0;
}
