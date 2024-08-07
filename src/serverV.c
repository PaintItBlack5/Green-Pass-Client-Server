#include<sys/types.h> /* predefined types */
#include<unistd.h> /* include unix standard library */
#include<arpa/inet.h> /* IP addresses conversion utililites */
#include<sys/socket.h> /* socket library */
#include<stdio.h> /* include standard I/O library */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>
#include "wrapper.h"
#define EXPIRATION6 15552000     // scadenza (rinnovo gp): 6 mesi (in secondi)
#define EXPIRATION2 172800        // scadenza (tampone): 2 giorni (in secondi)


struct green_pass {
  char ts[21]; // numero tessera sanitaria
  time_t scadenza; // validità green pass
  int servizio; // da richiedere al server
};

int main(int argc, char **argv){

  int list_fd, conn_fd;
  struct sockaddr_in serv_add;
  struct green_pass received;
  struct green_pass temp;
  pid_t pid;
  int zero = 0;
  FILE *file = fopen("file", "rb+");
  sem_t* access = sem_open("semaphore", O_CREAT, O_RDWR, 1);
  // semaforo per gestire gli accessi concorrenti al file


  list_fd = Socket(AF_INET,SOCK_STREAM,0);

  serv_add.sin_family = AF_INET;
  serv_add.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_add.sin_port = htons(1025);

  Bind(list_fd, (struct sockaddr *) &serv_add, sizeof(serv_add));

  Listen(list_fd,1025);

  while(1){
        conn_fd = Accept(list_fd, (struct sockaddr *) NULL, NULL);

        if((pid = fork()) < 0){
            perror("fork error");
            exit(-1);
        }


     if(pid==0){
        close(list_fd);

        if( read(conn_fd, &received, sizeof(received)) != sizeof(received)){
            perror("read");
            exit(1);
        }

    switch (received.servizio) {
    case -1:    // VACCINAZIONE
        // Il centro vaccinale richiede di aggiungere un green pass

        // controllo che non esista giá un green pass associato a questa tessera sanitaria
        fseek(file, 0, SEEK_SET); // dall'inizio del file:
        while(fread(&temp, sizeof(struct green_pass), 1, file) == 1){ // per ogni altro gp memorizzato sul file
            if(strcmp(temp.ts, received.ts) == 0){ // se è associato alla stessa tessera (è già presente):
                fseek(file, -(sizeof(struct green_pass)), SEEK_CUR); // riavvolgi di un gp
                sem_wait(access);
                fwrite(&received, sizeof(struct green_pass), 1, file); // sovrascrivi il precedente
                sem_post(access);
                printf("Tessera Sanitaria: %s aggiornata\n", received.ts);
                printf("Scadenza: %.24s\r\n", ctime(&received.scadenza));

                printf("--------\n");
                close(conn_fd);
                exit(0);
                break;
            }
        }

        // Altrimenti - Nuovo gp
        sem_post(access);
        fseek(file, 0, SEEK_END); // aggiungi in coda al file
        sem_wait(access);
        fwrite(&received, sizeof(struct green_pass), 1, file);
        sem_post(access);
        printf("Tessera Sanitaria: %s\n",received.ts);
        printf("Scadenza: %.24s\r\n", ctime(&received.scadenza));

        printf("--------\n");

        close(conn_fd);
        exit(0); // termina processo figlio
        break;

    case 0:     // VERIFICA DA PARTE DEI CITTADINI
        // Il clientS, tramite il serverG, richiede la verifica di un green pass (associato alla tessera sanitaria passata)

        fseek(file, 0, SEEK_SET); // dall'inizo del file:
        while(fread(&temp, sizeof(struct green_pass), 1, file) == 1){ // per ogni gp memorizzato sul file
            if(strcmp(temp.ts, received.ts) == 0){ // se è associato alla stessa tessera (è già presente):
                sem_wait(access);
                if(write(conn_fd, &temp, sizeof(struct green_pass)) != sizeof(struct green_pass)){ // restituisci il gp richiesto
                    sem_post(access);
                    perror("write");
                    exit(1);
                }else{ // altrimenti il processo figlio libera risorse e connessione e termina...
                    sem_post(access);
                    close(conn_fd);
                    exit(0);
                }
            }
        }

        // ...e il processo padre comunica al Server G che non è presente

        if(write(conn_fd, &zero, sizeof(int)) != sizeof(int)){
            perror("write");
            exit(1);
        }
        close(conn_fd);
        exit(0);
        break;

    case 1:     // TAMPONE O COMUNICAZIONE GUARIGIONE
        // Il Client T, tramite Server G, richiede la convalida di un green pass (associato alla tessera sanitaria passata)

        int cgp = 0; // counter di green pass all'interno del file (offset, scostamento)
        printf("Convalida green pass in corso...\n");

        fseek(file, 0, SEEK_SET); // dall'inizo del file:
        while(fread(&temp, sizeof(struct green_pass), 1, file) == 1){ // per ogni gp memorizzato sul file
            if(strcmp(temp.ts, received.ts) == 0){ // se è associato alla stessa tessera (è già presente):
                if(temp.servizio == 3){ // nel caso in cui, precedentemente, sia stato aggiunto come gp da tampone
                    temp.scadenza = time(NULL) + EXPIRATION2; // imposta 2 giorni di validità
                }else{
                    temp.scadenza = time(NULL) + EXPIRATION6; // altrimenti 6 mesi di validità (guarigione)
                }
                printf("Green Pass convalidato!\n");
                // sovrascrivi nella opportuna posizione all'interno del file
                fseek(file, cgp*sizeof(struct green_pass), SEEK_SET);
                sem_wait(access);
                fwrite(&temp, sizeof(struct green_pass), 1, file);
                sem_post(access);
                close(conn_fd);
                exit(0);
            }else { cgp++; } // passa al successivo gp nel file
        }
        sem_post(access);

        // nel caso in cui non sia nel file dei vaccinati, la tessera sanitaria ottiene un green pass valido 2 giorni (ha effettuato un tampone)
        strcpy(temp.ts, received.ts);
        temp.scadenza = time(NULL) + EXPIRATION2;
        temp.servizio = 3;
        printf("Tessera Sanitaria: %s\n", temp.ts);
        printf("Scadenza: %.24s\r\n", ctime(&temp.scadenza));
        sem_wait(access);
        fseek(file,0,SEEK_END); // aggiungi in coda
        fwrite(&temp, sizeof(struct green_pass), 1, file);
        sem_post(access);

        close(conn_fd);
        exit(0);
        break;

    case 2:     // CONTAGIO
        // Il Client T, tramite Server G, richiede l'invalida di un green pass (associato alla tessera sanitaria passata)

        printf("Invalidamento green pass in corso...\n");
        int cgp = 0; // counter di green pass all'interno del file (offset, scostamento)
        fseek(file, 0, SEEK_SET); // dall'inizo del file:
        while(fread(&temp, sizeof(struct green_pass), 1, file) == 1){ // per ogni gp memorizzato sul file
            if(strcmp(temp.ts, received.ts) == 0){ // se è associato alla stessa tessera (è già presente):
                temp.scadenza = time(NULL) - EXPIRATION2;
                printf("Green Pass invalidato!\n");
                // sovrascrivi nella opportuna posizione all'interno del file
                fseek(file, cgp*sizeof(struct green_pass), SEEK_SET);
                sem_wait(access);
                fwrite(&temp, sizeof(struct green_pass), 1, file);
                sem_post(access);
                close(conn_fd);
                exit(0);
            }else { cgp++; } // passa al successivo gp nel file
        }
        sem_post(access);

        // nel caso in cui non sia nel file dei vaccinati, la tessera sanitaria viene inserita comunque
        // (una volta guarito avrá il greenPass da guarigione da Covid)

        strcpy(temp.ts, received.ts);
        temp.scadenza = time(NULL) - EXPIRATION2;
        printf("Tessera Sanitaria: %s\n", temp.ts);
        printf("Scadenza: %.24s\r\n", ctime(&temp.scadenza));
        sem_wait(access);
        fseek(file, 0, SEEK_END);
        fwrite(&temp,sizeof(struct green_pass), 1, file);
        sem_post(access);

        break;
    } // end switch

    // Infine libera risorse e connessioni
    fclose(file);
    sem_unlink("semaphore");
    close(conn_fd);
    exit(0);
    } // end child
    else{ close(conn_fd); } // padre chiude la connessione e resta in ascolto per altre nuove
  } // end loop
  exit(0);
} // end Server V
