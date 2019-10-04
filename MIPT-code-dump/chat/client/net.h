#ifndef net_h_
#define net_h_

#include <stdio.h>

int tcp_open(const char *hostname, const char *service, FILE *err);

#endif
