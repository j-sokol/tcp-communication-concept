#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>


#define EOL "\r\n" 
#define BUFFSIZE 128
#define FORWARD 102
#define LEFT 103
#define RIGHT 104

int SRV_IP_PORT = 12019;
int PASV_FD = -1;
int DATACON_FD = -1;
int PASV_DATACON_FD = -1;
int LISTEN_QUEUE_LENGTH = 500;
int QUIT = 0;
int MAX_LINE = PATH_MAX;

struct location {
    int valid;
    int x;
    int y;
};

static int 
send_msg(int fd, char *msg, int len)   
{   
    int n, written = 0, left = len;   
    while (1) {   
        n = write(fd, msg + written, left);   
        if (n < 0) {   
            if (errno == EINTR)   
                continue;   
            return n;   
        }   
        if (n < left) {   
            left -= n;   
            written += n;   
            continue;   
        }   
        return len;   
    }   
}   

ssize_t 
readLine(int fd, void *buffer, size_t n)
{
    struct timeval timeout; 
    fd_set sockets;
    int ret;

    ssize_t numRead;                   
    size_t totRead;                   
    char *buf;
    char ch;

    FD_ZERO(&sockets);
    FD_SET(fd, &sockets);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }
    buf = buffer;          
    totRead = 0;
    for (;;) {
        ret = select(fd + 1, &sockets, NULL, NULL, &timeout);
        if(ret < 0)
        {
            perror("select()");
            return -1;
        }
        if(!FD_ISSET(fd, &sockets))
        {
            printf("Connection timeout\n");
            return -2;
        }
        numRead = read(fd, &ch, 1);

        if (numRead == -1) {
            if (errno == EINTR)   
                continue;
            else  return -1;           

        } else if (numRead == 0) {   
            if (totRead == 0)  return 0;
            else  break;

        } else {                        
            if (totRead < n - 1) {  
                totRead++;
                *buf++ = ch;
            }
            if (ch == '\n') break;
        }
    }
    *buf = '\0';
    return totRead;
}

static int recv_msg(int fd, char buf[], int len)   
{   
    int j = readLine (fd, buf, len);
    return j;
}   
static const char * get_response_str (int num)
{
    switch (num){
        case 100:
            return "100 LOGIN\r\n";
        case 101:
            return "101 PASSWORD\r\n";
        case 200:
            return "200 OK\r\n";
        case 300:
            return "300 LOGIN FAILED\r\n";
        case 102:
            return "102 MOVE\r\n";
        case 103:
            return "103 TURN LEFT\r\n";
        case 104:
            return "104 TURN RIGHT\r\n";
        case 105:
            return "105 GET MESSAGE\r\n";
        case 301:
            return "301 SYNTAX ERROR\r\n";
        case 302:
            return "302 LOGIC ERROR\r\n";
    }
    return "301 SYNTAX ERROR\r\n";

}

static int 
send_response ( int fd, int num, ...)
{
    const char *cp = get_response_str(num);   
    va_list ap;   
    char buf[BUFFSIZE];   
    printf("Sending response num: %i\n", num);

    if (!cp) {   
        fprintf(stderr, "get_response(%d): ", num);  
        return -1;
    }   
    va_start(ap, num);   
    vsnprintf(buf, sizeof(buf), cp, ap);   
    va_end(ap);   

    if (send_msg(fd, buf, strlen(buf)) != strlen(buf)) {   
        fprintf(stderr, "Partial message sent.%d\n", num);     
        return -1;
    }   
        fprintf(stderr, "Response %d sent.\n", num);     
    return 0;

}

static char * 
generate_password (char * username)
{
    int num_password = 0;
    static char password[BUFFSIZE];
    for (int i = 0; i < strlen(username) && username[i] != '\r'; ++i)
    {
        num_password += (int) username[i];
    }
    sprintf(password, "%d\r\n", num_password);
    return password;
}

static int 
recharge_sequence (int connfd)
{
    struct timeval timeout; 
    fd_set sockets;
    int ret;
    char buf[BUFFSIZE];   
    int buflen;   

    FD_ZERO(&sockets);
    FD_SET(connfd, &sockets);

    timeout.tv_sec = 6;
    timeout.tv_usec = 0;

    ret = select(connfd + 1, &sockets, NULL, NULL, &timeout);
    if(ret < 0) {
        perror("select()");
        return -1;
    }
    if(!FD_ISSET(connfd, &sockets)){
        printf("Connection timeout\n");
        return -2;
    }

    buflen = readLine (connfd, buf, BUFFSIZE);
    if (buflen < 0) {   
        fprintf(stderr, "Error receiving message\n");
        return -1;
    }   

    if (buflen == 0) return -1;   
    buf[buflen] = '\0';   
    if ( strncmp (buf, "FULL POWER\r\n", 8) != 0)
    {
        if (send_response(connfd, 302)) {  
            close(connfd);   
        }   
        return -2;   
    }
    return 0;
}

static int 
authenticate_connection (int connfd)
{
    char username[BUFFSIZE];   
    char password[BUFFSIZE];   
    int buflen;   

    if (send_response(connfd, 100)) {   /// send 200 USERNAME
        fprintf(stderr, "send_response():\n");     
        return -1;   
    }   

    while (1){
        buflen = recv_msg(connfd, username, sizeof(username));   
        if (buflen < 0) {   
            fprintf(stderr, "Error receiving message\n");
            if (buflen == -2)
                send_response(connfd, 301);
            return -1;
        }   
        if (buflen > 100) {   /// Username is longer than 100 chars
            send_response(connfd, 301);
            return -1;
        }
        if (buflen == 0) return -1;   

        username[buflen] = '\0';
        if ( strncmp (username, "RECHARGING", 8) == 0){
            if ( recharge_sequence (connfd)){
                return -1;
            }
        }  else break;
    }

    if (send_response(connfd, 101)) {   /// send 201 PASSWORD
        fprintf(stderr, "send_response():\n");     
        return -1;   
    }   

    while (1) {
        buflen = recv_msg(connfd, password, sizeof(password)); 
        if (buflen < 0) {   
            fprintf(stderr, "Error receiving message\n");
            return -1;
        }   
        if (buflen > 100) {
            send_response(connfd, 301);
            return -1;
        }
        if (buflen == 0) return -1;   

        password[buflen] = '\0';
        if ( strncmp (password, "RECHARGING\r\n", 10) == 0) {
            if ( recharge_sequence (connfd)){
                return -1;
            }
        } else break;
    }
    if ( strcmp (password, generate_password (username)) != 0) {
        send_response(connfd, 300);
        fprintf(stderr, "Password mismatch.\n");     
        return -2;   
    } else if (send_response(connfd, 200)) {   
            fprintf(stderr, "send_response()\n");     
            return -1;   
        }   
    return 0;
}
static struct location 
get_location (int connfd)
{
    struct location loc;
    char buf[BUFFSIZE];   
    int buflen;   

    while (1){
        buflen = recv_msg(connfd, buf, sizeof(buf));   
        if (buflen < 0) {   
            fprintf(stderr, "Error receiving message: %s\n", buf);
            loc.valid = -1;
            return loc;
        }   
        if (buflen == 0)
        {
            fprintf(stderr, "Buffer is empty.\n");
            loc.valid = -1;
            return loc;
        }   
        buf[buflen] = '\0';  
        if ( strncmp (buf, "RECHARGING\r\n", 10) == 0){
            int ret = recharge_sequence (connfd);
            if (ret == -1){
                fprintf(stderr, "Recharge sequence failed.\n");
                loc.valid = -1;
                return loc;
            }
            if (ret == -2) {
                fprintf(stderr, "Logical error.\n");
                loc.valid = -2;
                return loc;
            }
        } else {
            char term;        
            if ( sscanf ( buf, "OK %d %d%c", &loc.x, &loc.y, &term) != 2 && term != '\r') {
                fprintf(stderr, "Error scanning location.\n");
                loc.valid = -1;
                return loc;
            }
            loc.valid = 0;
            return loc;
        }
    }
}

static int 
get_direction ( struct location * prev, struct location * actual )  /// AI logic
{
    if (actual->y == 0 && actual->x == 0) 
        return FORWARD; 
    if ((abs(actual->x) < abs(prev->x) && actual->x != 0) || (abs(actual->y) < abs(prev->y) && actual->y != 0))
        return FORWARD;

    if (actual->x > 0 && actual->x > prev->x) 
        return LEFT; /// LEFT
    if (actual->x < 0 && actual->x < prev->x) 
        return LEFT;
    if (actual->y > 0 && actual->y > prev->y)
        return LEFT; /// LEFT
    if (actual->y < 0 && actual->y < prev->y) 
        return LEFT; /// LEFT

    if (actual->x == 0 && actual->x < prev->x) /// in right direction
        return LEFT; 
    if (actual->x == 0 && actual->x > prev->x) /// in right direction
        return RIGHT; 

    if (actual->y == 0 && actual->y < prev->y && actual->x > 0) 
        return RIGHT; 
    if (actual->y == 0 && actual->y > prev->y && actual->x > 0)
        return LEFT; 
    if (actual->y == 0 && actual->y < prev->y && actual->x < 0)
        return LEFT; 
    if (actual->y == 0 && actual->y > prev->y && actual->x < 0) 
        return RIGHT; 
    return FORWARD;

}
static int 
navigate_robot ( int connfd )
{
    struct location prev_loc;
    struct location actual_loc;
    /// let robot go one step forward first.
    if (send_response(connfd, FORWARD)) {   
        fprintf(stderr, "send_response():\n");     
        return -1;   
    }   
    prev_loc = get_location (connfd);
    if ( prev_loc.valid == -1 )
    {
        send_response(connfd, 301);
        fprintf(stderr, "Location not valid.\n");     
        return -1; 
    }
    if ( prev_loc.valid == -2 ) return -1;
    actual_loc.x = 0;
    actual_loc.y = 0;
    while ( 1 )
    {
        if (send_response(connfd, get_direction(&prev_loc, &actual_loc))) {   
            fprintf(stderr, "send_response():\n");     
            return -1;   
        }
        if (! (actual_loc.x == 0 && actual_loc.y == 0))       
            prev_loc = actual_loc; 
        actual_loc = get_location (connfd);
        if ( actual_loc.valid  == -1 )
        {
            send_response(connfd, 301);
            fprintf(stderr, "Location not valid.\n");     
            return -1; 
        }
        if ( actual_loc.valid == -2 ) return -1;

        if ( actual_loc.x == 0 && actual_loc.y == 0)
            break;
    }
    return 0;
}
static int 
receive_robot_message ( int connfd )
{
    char buf[BUFFSIZE];   
    int buflen;   

    if (send_response(connfd, 105)) {   
        fprintf(stderr, "send_response():\n");     
        return -1;   
    }   
    buflen = recv_msg(connfd, buf, sizeof(buf));   
    if (buflen < 0) {   
        fprintf(stderr, "Error recv_msg\n");
        return -1;
    }   

    if (buflen == 0) return -1;   
    buf[buflen] = '\0';  
    if (buflen > 100) {
        send_response(connfd, 301);
        return -1;
    }
    printf("Secret message from client is: %s\n", buf); 
    if (send_response(connfd, 200)) {   
        fprintf(stderr, "send_response():\n");     
        return -1;   
    }   
    return 0;
}

static int 
handle_connection (int connfd)
{
    if (authenticate_connection(connfd)) {
        close(connfd);   
        fprintf(stderr, "Authentication failed.\n");     
        return -1;   
    }

    if (navigate_robot(connfd)) {
        close(connfd);   
        fprintf(stderr, "Navigation failed.\n");     
        return -1;   
    }

    if (receive_robot_message(connfd)) {
        close(connfd);   
        fprintf(stderr, "Final stage failed.\n");     
        return -1;   
    }

    return 0;
}

static int 
init_server ()  
{   
    int fd;   
    int on = 1;   
    int err;   
    struct sockaddr_in srvaddr;   

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {   
        fprintf(stderr, "socket():\n");     
        return -1;
    }   

    err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));    
    if (err < 0) {   
        fprintf(stderr, "setsockopt():\n");     
        close(fd);   
        return -1;   
    }   

    memset(&srvaddr, 0, sizeof(srvaddr));   
    srvaddr.sin_family = AF_INET;   
    srvaddr.sin_port = htons(SRV_IP_PORT);
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);   
    err = bind(fd, (struct sockaddr*)&srvaddr, sizeof(srvaddr));   
    if (err < 0) {   
        fprintf(stderr, "bind:\n");   
        close(fd);   
        return -1;   
    }   
    /* accepting connections */
    err = listen(fd, LISTEN_QUEUE_LENGTH);   
    if (err < 0) {   
        fprintf(stderr, "listen():\n");   
        close(fd);   
        return -1;   
    }   
    return fd;
}

static int 
server_loop (int listenfd)   
{   
    int connfd;   
    int pid;   

    printf("Waiting for client connections...\n");   
    while (1) {   
        connfd = accept(listenfd, NULL, 0);   
        if (connfd < 0) {   
            continue;   
        }   
        printf("Connection received...\n");

        if ((pid = fork()) < 0) {   
            fprintf(stderr, "fork():\n");   
            close(connfd);   
            continue;   
        }   
        if (pid > 0) { /* parent */   
            close(connfd);   
            continue;   
        }   
        /* child */   
        close(listenfd);   
        if (handle_connection (connfd))   
            exit(-1);   
        exit(0);   
    }   

    return 0;
}   

int 
main(int argc, const char *argv[])
{
    server_loop(init_server());
    return 0;
}
