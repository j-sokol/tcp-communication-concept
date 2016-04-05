#ifndef FLUX_UTILS_H
#define FLUX_UTILS_H

#include <stdio.h>

#define logstd(FORMAT, ...) flux_log(stdout, __func__, "[log]", FORMAT, ##__VA_ARGS__)
#define logerr(FORMAT, ...) flux_log(stderr, __func__, "[err]", FORMAT, ##__VA_ARGS__)

void flux_log(FILE * stream, const char * func, const char * prefix, const char * format, ...);
void printport(int socketfd);

#endif
