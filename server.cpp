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
	void init (int);
	bool authenticate ();
	std::string receive_data ();
	bool send_data ( std::string text );
	std::string receive ();


	int c;
	pid_t pid;
	struct sockaddr_in remote_address;
	socklen_t size;


	/// timeout & select shit
	struct timeval timeout;
	fd_set sockets;
	int retval;
	char buffer[BUFFER_SIZE];

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
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;

}

bool Connection::authenticate ()
{
	this-> send_data ( "100 LOGIN\r\n");
	std::string input = this-> receive_data ();
	printf("login is: %s\n", input.c_str());
	this-> send_data ( "101 PASSWORD\r\n");
	input = this-> receive_data ();
	printf("pass is: %s\n", input.c_str());

	return true;
}
bool Connection::send_data ( std::string text )
{
	if(send(c, text.c_str(), text.size(), 0) < 0)
	{
		perror("Can't send data");
		close(c);
		return false;
	}
	return true;
}
std::string Connection::receive ()
{

}

std::string Connection::receive_data ()
{
	FD_ZERO(&sockets);
	FD_SET(c, &sockets);
	std::string text;
	int bytesRead;

	retval = select(c + 1, &sockets, NULL, NULL, &timeout);
	if(retval < 0)
	{
		perror("select err");
		close(c);
		return "";
	}
	if(!FD_ISSET(c, &sockets))
	{
		printf("connection timeout\n");
		close(c);
		return "";
	}

	bytesRead = recv(c, buffer, BUFFER_SIZE, 0);
	if(bytesRead <= 0)
	{
		perror("socket read err");
		close(c);
		return "";
	}
	buffer[bytesRead] = '\0';
	text. assign ( buffer );

	return text;
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
			if (! con.authenticate() )
			{
				printf("Failed to authenticate. Exiting...\n");
				break;
			}
			printf("in loop\n");

			while ( 1 )
			{
				std::string received_text = con.receive_data();
				if ( received_text == "")
					break;

				if(! strcmp ( "end" , con.buffer))
				{
					printf("Ending the server instance\n");
					break;
				}
				if (! strcmp ( "ahoj" , con.buffer))
				{
					printf("this if\n");
					con.send_data ( "hovno");
				}

				printf("%s\n", received_text.c_str() );

			}
			close ( con.c );
			return 0;
		}


	}

	return 0;
}
