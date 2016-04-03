#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h> // socket(), bind(), connect(), listen()
#include <unistd.h> // close(), read(), write()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // htons(), htonl()
#include <strings.h> // bzero()
#include <string.h>
#include <iostream>
#include <string>


#define BUFFER_SIZE 10240




int main(int argc, char const *argv[])
{
	printf("Starting client\n");
	if (argc > 3 || argc < 3)
	{
		printf("Usage: <server> <port>\n");
		return 1;
	}

	int port = atoi ( argv [2] );
	/// create socket
	int s = socket(AF_INET, SOCK_STREAM, 0);

	socklen_t sockAddrSize;

	struct sockaddr_in serverAddr;

	sockAddrSize = sizeof(struct sockaddr_in);
	bzero((char *) &serverAddr, sockAddrSize);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	struct hostent *host;
	host = gethostbyname(argv[1]); // <netdb.h>
	memcpy(&serverAddr.sin_addr, host->h_addr,
	host->h_length); // <string.h>

	if(connect(s, (struct sockaddr *) &serverAddr, sockAddrSize) < 0)
	{
		perror ( "Not able to connect:");
		close(s);
		return -1;
	}

//	char buffer [BUFFER_SIZE];
	char in_buffer [BUFFER_SIZE];
	std::string buffer;
//	strcpy(buffer, "bubak");
	while ( 1 )
	{
		printf("> ");
//		std::cin >> buffer ;
		std::getline(std::cin, buffer);

		//read(STDIN_FILENO, buffer, 100);
		if(send(s, buffer.c_str(), strlen(buffer.c_str()), 0) < 0)
		{
			perror("Can't send data");
			close(s);
			return -3;
		}

		int bytesRead = recv(s, in_buffer, BUFFER_SIZE, 0);

		in_buffer[bytesRead] = '\0';

		printf("%s\n", in_buffer);


		if(! strcmp (buffer.c_str(), "end"))
		{
			printf("Stopping the client\n");

			break;
		}
	}

	close(s);
	return 0;

}
