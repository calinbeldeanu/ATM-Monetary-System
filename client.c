#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFLEN 256

void error(char *msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[]) {
  FILE *log;
  int sockfd, udpsock, n  ,nr;
  int fdmax;
  int conex = 0;
  char *token;
  char pid[10];
  char last_card[10];
  char fileName[30];
  struct hostent *server;
  struct sockaddr_in client_addr , to_station;

  char buffer[BUFLEN];
  if (argc < 3) {
    fprintf(stderr, "Usage %s <IP_server> <port_server>\n", argv[0]);
    exit(0);
  }

  fd_set read_fds;    
  fd_set tmp_fds;            

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  udpsock = socket(PF_INET, SOCK_DGRAM, 0);

  if (udpsock < 0){
    error("-10 : Eroare la apel deschidere socket");
  }
  
  to_station.sin_family = AF_INET;
  to_station.sin_port = htons(atoi(argv[2]));
  inet_aton(argv[1], &to_station.sin_addr);


  if (sockfd < 0){
   error("-10 : Eroare la apel deschidere socket");
  }

  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(atoi(argv[2]));
  inet_aton(argv[1], &client_addr.sin_addr);

  if (connect(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0){
    error("-10 : Error la apel connect");
  }

  FD_SET(sockfd, &read_fds);
  FD_SET(STDIN_FILENO, &read_fds);
  FD_SET(udpsock , &read_fds);
  fdmax = udpsock;
  
  nr =getpid();
  memset(fileName , 0 , 30);
  memcpy(fileName , "client-<" ,9);
  memset(pid , 0 , 10);
  snprintf(pid , 10 , "%d" ,nr);
  strcat(fileName , pid);
  strcat(fileName , ">.log");  
  log = fopen(fileName , "w");

  while (1) {
    tmp_fds = read_fds;
    if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1){
      error("-10 : Error la apel select");
    }

    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &tmp_fds)) {
        if (i == STDIN_FILENO) {

          memset(buffer, 0, BUFLEN);
          fgets(buffer, BUFLEN - 1, stdin);
          if (strncmp(buffer, "quit",4) == 0) {
            strcpy(buffer ,"O sa inchid conexiunea !");
            n = send(sockfd, buffer, strlen(buffer), 0);
            fprintf(log, "%s","quit");
            close(sockfd);
            close(udpsock);
            fclose(log);
            return 0;
          }else if(strncmp(buffer, "login" , 5) == 0 && conex == 1){

            fprintf(log, "%s",buffer );
            printf("-2 : Sesiune deja deschisa\n");
            fprintf(log, "%s", "-2 : Sesiune deja deschisa");
            fprintf(log, "%s", "\n");

          }else if(strncmp(buffer, "logout", 6)== 0){
            if(conex == 1){
              conex = 0 ;
              n = send(sockfd, buffer, strlen(buffer), 0);
              fprintf(log, "%s", buffer);

            }else {
              fprintf(log, "%s",buffer );
              printf("-1 : Clientul nu este autentificat\n");
              fprintf(log, "%s", "-1 : Clientul nu este autentificat");
              fprintf(log, "%s", "\n");
            }
          }else if(strncmp(buffer , "unlock" ,6)==0){
           memset(buffer , 0 , 100);
           strcpy(buffer , last_card);
           sendto(udpsock, buffer, 13, 0, (struct sockaddr *) &to_station, sizeof(to_station));
           fprintf(log, "%s","unlock" );
           fprintf(log, "\n");

          }else{
          n = send(sockfd, buffer, strlen(buffer), 0);
          fprintf(log, "%s",buffer );
          if(strncmp(buffer, "login" , 5) == 0 && conex == 0 && strlen(buffer) > 15){
            token = strtok(buffer , " ");
            token = strtok(NULL , " ");
            strcpy(last_card , token);
          }
          }

          }else if (i == udpsock){
            memset(buffer , 0 ,BUFLEN);
            unsigned int recv = sizeof(struct sockaddr);
            recvfrom(udpsock, buffer, sizeof(buffer), 0, (struct sockaddr *)&to_station, &recv);
            printf("%s\n" , buffer);
            fprintf(log, "%s",buffer );
            fprintf(log, "\n");

            if(buffer[8] == 'T' ){
              memset(buffer , 0 , 100);
              fgets(buffer , BUFLEN-1 ,stdin);
              fprintf(log, "%s",buffer );
              sendto(udpsock, buffer, 17, 0, (struct sockaddr *) &to_station, sizeof(to_station));
            }
          }else{
          memset(buffer, 0, BUFLEN);
          if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
            if (n == 0) {
              printf("Serverul a inchis conexiunea\n");
              return 0;
            } else {
              error("-10 : Eroare la apel recv");
            }
            close(i);
            FD_CLR(i, &read_fds); 
          } else {
            printf("%s\n", buffer);
            fprintf(log, "%s",buffer );
            fprintf(log, "\n");
            if(buffer[5] == 'W'){
              conex = 1;
            }            
          }
        }
      }
    }
  }
  close(sockfd);
  return 0;
}


