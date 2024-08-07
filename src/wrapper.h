#ifndef WRAPPER_H_INCLUDED
#define WRAPPER_H_INCLUDED
#include <sys/socket.h>

// Funzioni contenitore per verificare le condizioni di uscita delle seguenti chiamate:
int Socket(int family,int type,int protocol);
int Connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int Bind(int sockfd, const struct sockaddr *servaddr,socklen_t addrlen);
int Listen(int sockfd,int queue_lenght);
int Accept(int sockfd,struct sockaddr *restrict clientaddr,socklen_t *restrict addr_dim);

#endif // WRAPPER_H_INCLUDED
