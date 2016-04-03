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


class Connection
{
public:
	Connection ( int s, int port ):
		s ( s ), port ( port ) { }
	void init (const char *);
	void communicate ();
	std::string receive_data ();
	bool authenticate ();

	bool send_data ( std::string text );


	socklen_t sockAddrSize;
	struct sockaddr_in serverAddr;
	struct hostent *host;
	int s;
	int port;


	char in_buffer [BUFFER_SIZE];
	std::string buffer_str;

	/// timeout & select shit
	struct timeval timeout;
	fd_set sockets;
	int retval;
	char buffer[BUFFER_SIZE];



};

void Connection::init ( const char * hostname)
{
	sockAddrSize = sizeof(struct sockaddr_in);
	bzero((char *) &serverAddr, sockAddrSize);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	host = gethostbyname(hostname); // <netdb.h>
	memcpy(&serverAddr.sin_addr, host->h_addr,
	host->h_length); // <string.h>


	if ( connect(s, (struct sockaddr *) &serverAddr, sockAddrSize) < 0)
	{
		perror ( "Not able to connect:");
		close(s);
		return;
	}

}
void Connection::communicate ()
{
	printf("> ");
//		std::cin >> buffer ;
	std::getline(std::cin, buffer_str);

	//read(STDIN_FILENO, buffer, 100);
	if(send(s, buffer_str.c_str(), strlen(buffer_str.c_str()), 0) < 0)
	{
		perror("Can't send data");
		close(s);
		return;
	}

	int bytesRead = recv(s, in_buffer, BUFFER_SIZE, 0);

	in_buffer[bytesRead] = '\0';

	printf("%s\n", in_buffer);


	if(! strcmp (buffer_str.c_str(), "end"))
	{
		printf("Stopping the client\n");
	}
}

bool Connection::send_data ( std::string text )
{
	if(send(s, text.c_str(), text.size(), 0) < 0)
	{
		perror("Can't send data");
		close(s);
		return false;
	}
	return true;
}
std::string Connection::receive_data ()
{
	FD_ZERO(&sockets);
	FD_SET(s, &sockets);
	std::string text;
	int bytesRead;

	retval = select(s + 1, &sockets, NULL, NULL, &timeout);
	if (retval < 0)
	{
		perror ("select err");
		close (s);
		return "";
	}
	if (!FD_ISSET(s, &sockets))
	{
		printf ("connection timeout\n");
		close (s);
		return "";
	}

	bytesRead = recv(s, buffer, BUFFER_SIZE, 0);
	if (bytesRead <= 0)
	{
		perror ("socket read err");
		close (s);
		return "";
	}
	buffer[bytesRead] = '\0';
	text. assign ( buffer );

	return text;
}
bool Connection::authenticate ()
{
	std::string input = this-> receive_data ();
//	printf("%s\n", input.c_str());
	if (input != "100 LOGIN\r\n")
		printf("not 100 LOGIN, but: %s\n", input.c_str());
	this->send_data("kokutek");
	input = this-> receive_data ();
	if (input != "101 PASSWORD\r\n")
		printf("not 101 PASSWORD, but: %s\n", input.c_str());
	this->send_data("pass_kokutek");

//		return false;
	return true;
}

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



	Connection con ( s, port);
	con.init ( argv[1] );

//	char buffer [BUFFER_SIZE];
//	strcpy(buffer, "bubak");
	if ( ! con.authenticate ())
	{
		printf("Failed to authenticate. Exiting...\n");
		return 1;
	}
	while ( 1 )
	{
		con.communicate();
	}

	close(s);
	return 0;

}
