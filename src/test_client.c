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
	int stop = 0;
	char sock[] = "/home/luca/workspace/ChatterBox/tmp/chatty_socket";
	int c_fd = openConnection( sock, 3, 2 );

	message_t my_msg;
	message_t server_msg;
	char tmp[512];



	int op;
	char name[MAX_NAME_LENGTH];
	printf( "Insert your nickname\n" );
	scanf( "%s", name );

	do
	{
		printf( "What do you want to do?\n" );
		scanf( " %d", &op );
		printf( "\n" );

		switch ( op )
		{
			case 1:
			{
				setHeader( &my_msg.hdr, REGISTER_OP, name );
				setData( &my_msg.data, "", NULL, 0 );
				sendRequest( c_fd, &my_msg );

				readHeader( c_fd, &server_msg.hdr );

				if( server_msg.hdr.op == OP_OK )
				{
					printf( "User %s registered\n", my_msg.hdr.sender );

					if (readData(c_fd, &server_msg.data) <= 0) {
						perror("reply data");
						return -1;
					}

					int nusers = server_msg.data.hdr.len / (MAX_NAME_LENGTH+1);
					assert(nusers > 0);
					printf("Lista utenti online:\n");

					for(int i=0,p=0;i<nusers; ++i, p+=(MAX_NAME_LENGTH+1) )
					{
						printf(" %s\n", &server_msg.data.buf[p]);
					}


				}
				else
				{
					if( server_msg.hdr.op == OP_NICK_ALREADY  )
						printf( "User %s already registered\n", my_msg.hdr.sender );
					else
						printf( "User %s unknown\n", my_msg.hdr.sender );
					stop = 1;
				}

			}break;


			case 2:
				{
					setHeader( &my_msg.hdr, POSTTXT_OP, name );
					char mex[500];
					char rec[MAX_NAME_LENGTH];

					printf( "Insert reciver name\n" );
					scanf( "%s", rec );

					printf( "Insert your message\n" );
					scanf( "%s", mex );
					setData( &my_msg.data, rec, mex, sizeof(mex) );

					sendRequest( c_fd, &my_msg );

					readHeader( c_fd, &server_msg.hdr );
					if( server_msg.hdr.op == OP_OK )
					{
						printf( "Message sent\n" );
					}
					else
					{
						printf( "Problem to receive message\n" );
					}
				}break;

			case 3:
				{
					setHeader( &my_msg.hdr, CONNECT_OP, name );
					setData( &my_msg.data, "", NULL, 0 );
					sendRequest( c_fd, &my_msg );

					readHeader( c_fd, &server_msg.hdr );
					if( server_msg.hdr.op == OP_OK )
					{
						printf( "User %s connected\n", my_msg.hdr.sender );


						if (readData(c_fd, &server_msg.data) <= 0) {
							perror("reply data");
							return -1;
						}

						int nusers = server_msg.data.hdr.len / (MAX_NAME_LENGTH+1);
						assert(nusers > 0);
						printf("Lista utenti online:\n");
						for(int i=0,p=0;i<nusers; ++i, p+=(MAX_NAME_LENGTH+1)) {
							printf(" %s\n", &server_msg.data.buf[p]);
						}
					}
					else
					{
						printf( "User %s not been connected\n", my_msg.hdr.sender );
					}

				}break;

			case 4:
				{
					setHeader( &my_msg.hdr, DISCONNECT_OP, name );
					setData( &my_msg.data, "", NULL, 0 );

					sendRequest( c_fd, &my_msg );

					readHeader( c_fd, &server_msg.hdr );
					if( server_msg.hdr.op == OP_OK )
					{
						stop = 1;
					}
					else
					{
						printf( "STI CAZZI" );
					}
				}break;


			case 5:
			{
				setHeader( &my_msg.hdr, USRLIST_OP, name );
				setData( &my_msg.data, "", NULL, 0 );
				sendRequest( c_fd, &my_msg );

				readHeader( c_fd, &server_msg.hdr );
				if( server_msg.hdr.op == OP_OK )
				{

					if (readData(c_fd, &server_msg.data) <= 0) {
						perror("reply data");
						return -1;
					}

					int nusers = server_msg.data.hdr.len / (MAX_NAME_LENGTH+1);
					assert(nusers > 0);
					printf("Lista utenti online:\n");
					for(int i=0,p=0;i<nusers; ++i, p+=(MAX_NAME_LENGTH+1)) {
						printf(" %s\n", &server_msg.data.buf[p]);
					}
				}
			}break;

			case 6:
			{
				setHeader( &my_msg.hdr, UNREGISTER_OP, name );
				setData( &my_msg.data, "", NULL, 0 );

				sendRequest( c_fd, &my_msg );

				readHeader( c_fd, &server_msg.hdr );
				if( server_msg.hdr.op == OP_OK )
				{
					printf( "\nUSER DELETED\n" );
					stop = 1;
				}
				else
				{
					printf( "\nUSER NOT DELETED\n" );
					stop = 1;
				}

			}break;

			case 7:
			{
				printf( "INSERT GROUP NAME\n" );
				scanf( "%s", tmp );
				setHeader( &my_msg.hdr, CREATEGROUP_OP, name );
				setData( &my_msg.data, tmp, NULL, 0 );
				sendRequest( c_fd, &my_msg );

				readHeader( c_fd, &server_msg.hdr );
				if( server_msg.hdr.op == OP_OK )
				{
					printf( "GROUP CREATED\n" );
				}
				else
				{
					printf( "GROUP CAN NOT BE CREATED\n" );
					stop = 1;
				}

			}break;

			case 8:
			{
				printf( "INSERT GROUP NAME\n" );
				scanf( "%s", tmp );
				setHeader( &my_msg.hdr, ADDGROUP_OP, name );
				setData( &my_msg.data, tmp, NULL, 0 );
				sendRequest( c_fd, &my_msg );

				readHeader( c_fd, &server_msg.hdr );
				if( server_msg.hdr.op == OP_OK )
				{
					printf( "USER ADDED\n" );
				}
				else
				{
					printf( "%d\n", server_msg.hdr.op );
					stop = 1;
				}

			}break;


			case 9:
			{
				printf( "INSERT GROUP NAME\n" );
				scanf( "%s", tmp );
				setHeader( &my_msg.hdr, DELGROUP_OP, name );
				setData( &my_msg.data, tmp, NULL, 0 );
				sendRequest( c_fd, &my_msg );

				readHeader( c_fd, &server_msg.hdr );
				if( server_msg.hdr.op == OP_OK )
				{
					printf( "GROUP DELETED\n" );
				}
				else
				{
					printf( "%d\n", server_msg.hdr.op );
					stop = 1;
				}

			}break;

			default:
				{
					printf( "unknown OP\n" );
					stop = 1;
				}
		}

		printf( "\n\n" );
		op = 0;
	}while( !stop );




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

	sleep( 2 );

	close( c_fd );

	return 0;
}
