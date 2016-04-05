#include "utils.h"

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <arpa/inet.h>

void flux_log(FILE * stream, const char * func, const char * prefix, const char * format, ...)
{
    int buffsize = 64;
    char * buff = malloc(buffsize * sizeof(char));

    while(snprintf(buff, buffsize, "%s[%s] %s", prefix, func, format) >= buffsize)
    {
        buffsize *= 2;
        buff = realloc(buff, buffsize*sizeof(char));
        if(buff == NULL)
        {
            fprintf(stderr, "Error allocating memory for log msg! Exiting...\n");
            exit(-1);
        }
    }

    va_list argptr;
    va_start(argptr, format);
    vfprintf(stream, buff, argptr);
    va_end(argptr);
}

void printport(int socketfd)
{
    
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socketfd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        printf("port number %d\n", ntohs(sin.sin_port));
}
