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
    char tmp[50] = "ciao\040 come\040 stai?";
    setHeader( &msg.hdr, GETFILE_OP, "luca" );
    setData( &msg.data, "Roby", tmp, strlen(tmp) );
    if (sendRequest(c_fd, &msg) == -1)
    	return -1;
    else
    	printf( "header sent\n" );

    if( sendData( c_fd, &msg.data ) == -1 )
    	return -1;
    else
    	printf( "msg sent\n" );
//    send( c_fd, tmp, 50, 0 );



	printf("Client %d ha inviato il messaggio\n", getpid());


	close( c_fd );

	return 0;
}
