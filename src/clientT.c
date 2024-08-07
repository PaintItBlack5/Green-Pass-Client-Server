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

            /* MEDICO */
int main(int argc, char** argv){

    int sockfd,n;
    struct sockaddr_in servaddr;
    struct green_pass greenPass;
    char valid[] = "V";
    char invalid[] = "I";

    if(argc != 4){
        fprintf(stderr,"usage: %s <IPaddress> <TesseraSanitaria> <V or I>\n", argv[0]);
        exit(1);
    }

    if(strlen(argv[2]) != 20){
        fprintf(stderr,"Tessera Sanitaria non valida \n");
        exit(1);
    }

    strcpy(greenPass.tessera, argv[2]);
    if (strcmp(argv[3], valid) == 0)
        greenPass.servizio = 1;
    else if (strcmp(argv[3], invalid) == 0)
        greenPass.servizio = 2;
    else{
        printf("Scelta %s non valida...Riprovare...", argv[3]);
        exit(1);
    }

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1026);

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
        fprintf(stderr,"inet_pton error for %s\n",argv[1]);
        exit(1);
    }

    Connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    // Invio green pass e relativa richiesta a Server G
    if(write(sockfd, &greenPass, sizeof(greenPass)) != sizeof(greenPass)) {
        perror("write");
        exit(1);
    }
    printf("Richiesta Inoltrata...\n");

    // Risposte dal server V

    close(sockfd);
    exit(0);

} // end Client T
