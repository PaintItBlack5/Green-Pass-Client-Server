#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "wrapper.h"
#define EXPIRATION6 15552000     // scadenza: 6 mesi (in secondi)

struct green_pass {
  char ts[21]; // numero tessera sanitaria
  time_t scadenza; // validità green pass
  int servizio; // da richiedere al server
};


int main(int argc,char **argv){

  int listenfd, connfd, serverVfd;
  int n, logging = 1, n_client = 0;
  pid_t pid;
  struct green_pass greenPass;
  struct sockaddr_in servCV, serverV;

  if(argc != 2){
    fprintf(stderr,"usage: %s <IPaddress>\n", argv[0]);
    exit(1);
  }


  listenfd = Socket(AF_INET, SOCK_STREAM, 0); // creazione socket di ascolto (AddressFamilyInterNET, flusso TCP)
  // inizializzazione campi del server Centro Vaccinale
  servCV.sin_family = AF_INET;
  servCV.sin_addr.s_addr = htonl(INADDR_ANY); // conversione codifica (Long): host to network (to big endian)
  servCV.sin_port = htons(1024); // (Short)

  Bind(listenfd, (struct sockaddr *) &servCV, sizeof(servCV)); // associazione socket (fd) - indirizzo ip
  // il secondo argomento previsto è un indirizzo generico; è necessario un cast

  Listen(listenfd,1024); // socket in ascolto (coda di attesa di connessione: 1024)

  for( ; ; ) {

    connfd = Accept(listenfd, (struct sockaddr *) NULL, NULL);
    // nuova connessione = nuova socket (listenfd resta in ascolto per altre)
    n_client++;

    // processo figlio:
    if((pid = fork()) < 0){
      perror("fork error");
      exit( -1);
    }
    if(pid == 0){
        close(listenfd); // chiudi il descrittore in ascolto ereditato dal padre
        while((n = read(connfd, greenPass.ts, 20)) > 0 ) { // finchè ci sono numeri di TS, aggiungi green pass al Server V
            greenPass.ts[n] = 0;
            if (fputs(greenPass.ts, stdout) == EOF) {
                fprintf(stderr, "fputs error\n");
                exit(1);
            }
            greenPass.scadenza = time(NULL) + EXPIRATION6; // ripristina scadenza (6 mesi)
            greenPass.servizio = -1; // aggiunta green pass

            if(n<0){
                fprintf(stderr, "read error\n");
                exit(1);
            }
            // invio dati a Server V
            serverVfd = Socket(AF_INET, SOCK_STREAM, 0);
            serverV.sin_family = AF_INET;
            serverV.sin_port = htons(1025);

            if(inet_pton(AF_INET, argv[1], &serverV.sin_addr) < 0) {
                fprintf(stderr, "inet_pton error for %s\n", argv[1]);
                exit(1);
            }

            Connect(serverVfd, (struct sockaddr *)&serverV, sizeof(serverV));

            if(write(serverVfd, &greenPass, sizeof(greenPass)) != sizeof(greenPass)) {
                perror("write");
                exit(1);
            }
            printf(": Green Pass aggiunto con successo al server V!\n");

            close(serverVfd);
            close(connfd);
            exit(0); // il processo figlio chiude la connessione con il server V e termina
        } // fine while read
        exit(0);
    } // il processo figlio è terminato
    else {close(connfd);} // terminato il figlio, anche il padre chiude la connessione
    } // loop infinito
} // end Centro Vaccinale
