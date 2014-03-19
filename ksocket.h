#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "rtap-ko.h"

//struct socket;
//struct sockaddr;
//struct in_addr;
typedef struct socket * ksocket_t;


ksocket_t ksocket(int domain, int type, int protocol);
int kclose(ksocket_t socket);

ssize_t ksendto(ksocket_t socket, void *message, size_t length, int flags, const struct sockaddr *dest_addr, int dest_len);

unsigned int inet_addr(char* ip);

#endif
