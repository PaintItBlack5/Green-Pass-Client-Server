#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include<errno.h>
#include<netdb.h>
#include "wrapper.h"
#include <time.h>


struct green_pass {
    char ts[21];
    time_t scadenza;
    int servizio;
};

int main(int argc,char **argv) {

    int sockfd; // file descriptor per connessioni con Client S (cittadini)
    int listenfd; // file descriptor in ascolto di nuove connessioni (da Client S e T - cittadini e medici)
    int requestfd; // file descriptor per inviare richieste a Server V
    int n;
    struct green_pass greenPass;
    struct sockaddr_in servaddr; // per i client
    struct sockaddr_in requestaddr; // verso il Server V
    pid_t pid;

    if(argc != 2){
        fprintf(stderr,"usage: %s <IPaddress> \n",argv[0]);
        exit(1);
    }

    // LATO SERVER (comunicazioni con Client S e T)

    listenfd = Socket(AF_INET,SOCK_STREAM,0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(1026);

    Bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    Listen(listenfd,1026);

    for( ; ; ){

        sockfd = Accept(listenfd, (struct sockaddr *)NULL, NULL);

        if((pid = fork()) < 0 ){
            perror("fork error");
            exit(-1);
        }

        if(pid == 0){
            close(listenfd);

        // Ricezione green pass con associata richiesta di servizio
        while((n = read(sockfd, &greenPass, sizeof(greenPass))) > 0){
            greenPass.ts[21] = 0;
            printf("Tessera Sanitaria: %s\n",greenPass.ts);
            if(n<0){
                fprintf(stderr,"read error\n");
                exit(1);
            }


            // LATO CLIENT (comunicazioni con Server V)
            requestfd = Socket(AF_INET,SOCK_STREAM,0);
            requestaddr.sin_family = AF_INET;
            requestaddr.sin_port = htons(1025);

            if(inet_pton(AF_INET, argv[1], &requestaddr.sin_addr) < 0) {
                fprintf(stderr,"inet_pton error for %s\n", argv[1]);
                exit(1);
            }

            // Connessione e invio richiesta al Server V
            Connect(requestfd, (struct sockaddr *) &requestaddr, sizeof(requestaddr));
            if(write(requestfd,&greenPass,sizeof(greenPass)) != sizeof(greenPass)) {
                perror("write");
                exit(1);
            }
            printf("Richiesta Consegnata\n");
            printf("Servizio : %d\n", greenPass.servizio);

            // Leggendo la risposta del Server V (su requestfd), se la richiesta proviene da un cittadino (Client S - verifica green pass)...
            if(greenPass.servizio == 0){
                int r = read(requestfd, &greenPass, sizeof(greenPass));
                if( (r != sizeof(greenPass))  && (r != sizeof(int)) ){
                    perror("read");
                    exit(1);
                }else if (r < sizeof(greenPass))
                    printf("Green Pass non trovato\n");
                    else{ // altrimenti mostralo e inoltralo a S (su sockfd)
                        printf("Tessera Sanitaria: %s\n", greenPass.ts);
                        printf("Scadenza: %.24s\r\n", ctime(&greenPass.scadenza));

                        if( write(sockfd, &greenPass, sizeof(greenPass)) < sizeof(greenPass)){
                            perror("write");
                            exit(1);
                        }
                    }
                close(sockfd);
                close(requestfd);
                exit(0);

            // ...altrimenti se la richiesta proviene da un medico (Client T)
            } else if(greenPass.servizio == 1 || greenPass.servizio ==2){
                close(sockfd);
                close(requestfd);
                exit(0);
                }

        } // end ricezione green pass
        } // end child

    } // end loop

  close(sockfd);
  exit(0);
} // end Server G
