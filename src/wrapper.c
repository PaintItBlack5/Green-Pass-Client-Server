#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "wrapper.h"

// per migliorare la leggibilità del codice "incapsuliamo" i controlli sugli errori delle socket

int Socket(int family,int type,int protocol){
  int n;
  if((n = socket(family,type,protocol))<0){
    perror("socket");
    exit(1);
  }

  return n;
}

int Connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen){
  if(connect(sockfd,addr,addrlen) < 0){
    perror("connect");
    exit(1);
  }

}

int Bind(int sockfd, const struct sockaddr *servaddr,socklen_t addrlen){
  if(bind(sockfd,servaddr, addrlen) < 0){
    perror("bind");
    exit(1);
  }

}

int Listen(int sockfd,int queue_lenght){
  if(listen(sockfd,queue_lenght) < 0){
    perror("listen");
    exit(1);
  }

}

int Accept(int sockfd,struct sockaddr *restrict clientaddr,socklen_t *restrict addr_dim){
  int n;
  if((n = accept(sockfd,clientaddr, addr_dim))<0){
    perror("accept");
    exit(1);
  }
  return n;
}
