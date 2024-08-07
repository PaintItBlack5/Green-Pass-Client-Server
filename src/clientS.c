#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "wrapper.h"
#include <time.h>


struct green_pass {
    char ts[21];
    time_t scadenza;
    int servizio;
};

            /* CITTADINO */
int main(int argc, char** argv){
    int sockfd;
    struct sockaddr_in servaddr;
    struct green_pass greenPass;

    if(argc != 3){
        fprintf(stderr,"usage: %s <IPaddress> <TesseraSanitaria>\n",argv[0]);
        exit(1);
    }

    if(strlen(argv[2]) != 20){
        fprintf(stderr,"Tessera Sanitaria non valida \n");
        exit(1);
    }

    strcpy(greenPass.tessera, argv[2]);
    greenPass.servizio = 0;

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1026);

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
        fprintf(stderr,"inet_pton error for %s\n",argv[1]);
        exit(1);
    }

    Connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    // Invio green pass e relativa richiesta a Server G
    if(write(sockfd, &greenPass, sizeof(greenPass)) != sizeof(greenPass)) {
        perror("write");
        exit(1);
    }
    printf("Richiesta Inoltrata...\n");

    // Lettura risposta del Server G
    if(read(sockfd,&greenPass,sizeof(greenPass)) != sizeof(greenPass)){
        perror("read");
        exit(1);
    }

    // show
    printf("Tessera Sanitaria: %s\n", greenPass.ts);
    printf("Scadenza: %.24s\r\n", ctime(&greenPass.scadenza));

    if(greenPass.scadenza >= time(NULL))
        printf("Validita greenPass : VALIDO\n");
    else
        printf("Validita greenPass: NON VALIDO\n");

    close(sockfd);
    exit(0);

} // end client S
