#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "wrapper.h"
#include <time.h>

            /* PERSONALE DEL HUB VACCINALE */
int main(int argc,char **argv) {
    int sockfd, n;
    struct sockaddr_in servaddr;

    if(argc != 3){
        fprintf(stderr,"usage: %s <IPaddress> <TesseraSanitaria>\n",argv[0]);
        exit(1);
    }

    if(strlen(argv[2]) != 20){
        fprintf(stderr,"Tessera Sanitaria non valida \n");
        exit(1);
    }

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1024);

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
        fprintf(stderr,"inet_pton error for %s\n", argv[1]);
        exit(1);
    }

    Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if(write(sockfd, argv[2], strlen(argv[2])) != strlen(argv[2])) {
        perror("write");
        exit(1);
    }
    printf("Tessera Sanitaria Consegnata\n");
    close(sockfd);
}
