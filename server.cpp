#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> // socket(), bind(), connect(), listen()
#include <unistd.h> // close(), read(), write()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // htons(), htonl()
#include <strings.h> // bzero()
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <string>


#define BUFFER_SIZE 10240
#define TIMEOUT 10


class Connection
{
public:
	Connection();
	~Connection();

	void init (int);

	int c;
	pid_t pid;
	struct sockaddr_in remote_address;
	socklen_t size;

};

void Connection::init (int l)
{
	c = accept(l, (struct sockaddr *) &remote_address, &size);
	if (c < 0)
	{
		perror ("accept");
		close (l);
		return;
	} 
	pid = fork();

}

int main(int argc, char const *argv[])
{
	std::string rettext = "Hey buddy!";
	int l = socket(AF_INET, SOCK_STREAM, 0);
	if(l < 0)
	{
		perror("Cannot create socket!");
		return -1;
	}

	int port = atoi(argv[1]);
	if(port == 0)
	{
		printf("Usage: <port>\n");
		close(l);
		return -1;
	}

	struct sockaddr_in address;
	bzero(&address, sizeof (address));
	address.sin_family = AF_INET;
	address.sin_port = htons (port);
	address.sin_addr.s_addr = htonl (INADDR_ANY);

	// bind socket
	if (bind (l, (struct sockaddr *) &address, sizeof(address)) < 0)
	{
		perror ("bind");
		close (l);
		return -1;
	}

	// socket as pasive
	if (listen (l, 10) < 0)
	{
		perror ("listen");
		close (l);
		return -1;
	}


	while ( 1 )
	{
		Connection con;
		con.init(l);
		if ( con.pid == 0 )
		{
				struct timeval timeout;
				timeout.tv_sec = TIMEOUT;
				timeout.tv_usec = 0;
				fd_set sockets;
				int retval;
				char buffer[BUFFER_SIZE];

				while ( 1 )
				{
					FD_ZERO(&sockets);
					FD_SET(con.c, &sockets);

					retval = select(con.c + 1, &sockets, NULL, NULL, &timeout);
					if(retval < 0)
					{
						perror("select err");
						close(con.c);
						return -1;
					}
					if(!FD_ISSET(con.c, &sockets))
					{
						printf("connection timeout\n");
						close(con.c);
						return 0;
					}
					int bytesRead = recv(con.c, buffer, BUFFER_SIZE, 0);
					if(bytesRead <= 0)
					{
						perror("socket read err");
						close(con.c);
						return -3;
					}
					buffer[bytesRead] = '\0';

					if(! strcmp ( "end" , buffer))
					{
						printf("Ending the server instance\n");
						break;
					}
					if (! strcmp ( "ahoj" , buffer))
					{
						printf("this if\n");

						if (send(con.c, rettext.c_str(), strlen(rettext.c_str()), 0) < 0)
						{
							perror("Can't send data");
						}

					}

					printf("%s\n", buffer);

				}
				close ( con.c );
				return 0;
		}


	}

	return 0;
}
