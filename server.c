#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CLIENTS	10
#define BUFLEN 256

typedef struct client{
	char nume[13];
	char prenume[13];
	int  num_card;
	int  pin;
	char  secret_pass[17];
	double sold;
	int block;
	int active;
	int sock;
}client;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	 FILE *source;
	 int ok = 0;
	 int nrLines = 0;
	 int fdmax;	
	 int n, i, j , auxMoney;
     int sockfd, newsockfd, portno, clilen , udpsock;
	 char *token;
	 char money[20];
	 char line[100];
	 char auxBuffer[100];
     char buffer[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr , from_station;
     struct sockaddr udpsendserv;

    if (argc < 3) {
   		 fprintf(stderr, "Usage %s <port_server> <users_data_file>\n", argv[0]);
   		 exit(0);
  	}

     source = fopen(argv[2],"r");

     if(source == NULL){
     	error("-10 : Eroare la apel fopen");
     }

     if(fgets (line, 100 , source) != NULL){
     	nrLines = atoi(line);
     }

     client clienti[nrLines];

     for(int i = 0; i < nrLines ; i++){
     	fgets(line , 100 , source);
     	token = strtok(line," ");
     	strcpy(clienti[i].nume ,token);
     	token = strtok(NULL," ");
     	strcpy(clienti[i].prenume ,token);
     	token = strtok(NULL," ");
     	clienti[i].num_card = atoi(token);
     	token = strtok(NULL," ");
     	clienti[i].pin = atoi(token);
     	token = strtok(NULL," ");
     	strcpy(clienti[i].secret_pass ,token);
     	token = strtok(NULL , " ");
     	sscanf(token , "%lf" , &clienti[i].sold);
     	clienti[i].block = 0;
     	clienti[i].active = 0;
     	printf("%s\n", clienti[i].secret_pass);
     }
	fclose(source);

     fd_set read_fds;
     fd_set tmp_fds;		

     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     udpsock = socket(PF_INET, SOCK_DGRAM, 0);
     portno = atoi(argv[1]);

  	if (udpsock < 0){
    error("-10 : Eroare la apel deschidere socket");
  	}
  
  	
  	memset((char *) &from_station, 0, sizeof(from_station));
  	from_station.sin_family = AF_INET;
  	from_station.sin_addr.s_addr = INADDR_ANY;
  	from_station.sin_port = htons(portno);

  	if(bind(udpsock, (struct sockaddr *)&from_station, sizeof(struct sockaddr)) < 0){
  		error("-10 : Eroare la apel binding");
    }
  	  
    if (sockfd < 0){ 
       error("-10 : Eroare la apel deschidere socket");
    }

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0){ 
              error("-10 : Eroare la apel binding");
     }
     
     listen(sockfd, CLIENTS);
     FD_SET(udpsock , &read_fds);
     FD_SET(sockfd, &read_fds);
     FD_SET(STDIN_FILENO, &read_fds);
     fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1){
			error("-10 : Eroare la apel deschidere socket");
		}
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
			
				if (i == sockfd) {
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);			
					} else if (i == STDIN_FILENO) {
					scanf("%s", buffer);
					if (strcmp(buffer, "quit") == 0) {
						exit(0);
					}else{
					 printf("Unknown command\n");
					}
				
				} else if (i == udpsock){
					memset(buffer , 0 , 100);
					int j = 0 ;
					unsigned int recv = sizeof(struct sockaddr);
					recvfrom(udpsock, buffer, sizeof(buffer), 0, (struct sockaddr *)&udpsendserv, &recv);
					while(j < nrLines && ok == 0){
						if(clienti[j].num_card == atoi(buffer)){
							break;
						}else{
							j++;
						}
					}

					while(j < nrLines && ok == 1){
						if(strncmp(clienti[j].secret_pass, buffer , strlen(clienti[j].secret_pass))== 0){
							break;
						}else{
							j++;
						}
					}

					if(j<nrLines && ok == 0 && clienti[j].block == 3){
						memset(buffer , 0 , 100);
						ok = 1;
						strcpy(buffer , "UNLOCK> Trimite parola secreta ");
						sendto(udpsock, buffer,strlen(buffer), 0, (struct sockaddr *)&udpsendserv, sizeof(udpsendserv));
						
					}else if(j<nrLines && ok == 0){
						strcpy(buffer , "UNLOCK> -6 : Operatie esuata");
						sendto(udpsock, buffer,strlen(buffer), 0, (struct sockaddr *)&udpsendserv, sizeof(udpsendserv));
					}else if(j < nrLines && ok ==1){
						memset(buffer , 0 ,100);
						ok = 0;
						clienti[j].block = 0;
						strcpy(buffer , "UNLOCK> -5 : Client deblocat");
						sendto(udpsock, buffer, strlen(buffer), 0, (struct sockaddr *) &udpsendserv, sizeof(udpsendserv));
					}else if (ok == 0){
						memset(buffer , 0 , 100);
						strcpy(buffer , "UNLOCK> -4 : Numar card inexistent");
						sendto(udpsock, buffer, strlen(buffer), 0, (struct sockaddr *)&udpsendserv, sizeof(udpsendserv));
					}else if(ok == 1){
						memset(buffer , 0 , 100);
						strcpy(buffer , "UNLOCK> -7 : Deblocare esuata");
						ok = 0;
						sendto(udpsock, buffer, strlen(buffer), 0, (struct sockaddr *) &udpsendserv, sizeof(udpsendserv));
					}
				}else{
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							printf("selectserver: socket %d hung up\n", i);
							int j = 0;
							while(j < nrLines){
								if(clienti[j].sock == i){
									clienti[j].active = 0;
									clienti[j].sock = -1;
								}else{
									j++;
								}
							}

						} else {
							error("-10 : Eroare la apel recv");
						}
						close(i); 
						FD_CLR(i, &read_fds); 
					} 
					
					else {
						memset(auxBuffer , 0 , 100);
						if(strncmp(buffer , "login" , 5) == 0){
								int j =0; 
								int auxPin = 0;
								int auxNr_card = 0;
								char command[5];
								sscanf(buffer , "%s %d %d",command ,&auxNr_card , &auxPin);
								

							while(j<nrLines){
								if(auxNr_card == clienti[j].num_card ){

									if(clienti[j].active ==1 ){
										strcpy(auxBuffer , "ATM> -2 : Sesiune deja deschisa");
										send(i , auxBuffer , strlen(auxBuffer), 0);
										j++;
										break;
									}else if(auxPin == clienti[j].pin && clienti[j].block != 3){
										clienti[j].block = 0;
										clienti[j].sock = i;
										strcpy(auxBuffer , "ATM> Welcome ");
										strcat(auxBuffer, clienti[j].nume);
										strcat(auxBuffer , " ");
										strcat(auxBuffer , clienti[j].prenume);
										clienti[j].active = 1;
										send(i , auxBuffer , strlen(auxBuffer) , 0);
										j++;
										break;
									
									}else if(auxPin != clienti[j].pin && clienti[j].block == 2){
										clienti[j].block ++;
										strcpy(auxBuffer , "ATM> -5 : Card blocat");
										send(i , auxBuffer , strlen(auxBuffer), 0);
										j++;
										break;
									}else if(auxPin == clienti[j].pin && clienti[j].block == 3){
										strcpy(auxBuffer , "ATM> -5 : Card blocat");
										send(i , auxBuffer , strlen(auxBuffer), 0);
										j++;
										break;
									}else if(auxPin != clienti[j].pin && clienti[j].block == 3){
										strcpy(auxBuffer , "ATM> -5 : Card blocat");
										send(i , auxBuffer , strlen(auxBuffer), 0);
										j++;
										break;

									}else if(auxPin != clienti[j].pin && clienti[j].block != 2){
										clienti[j].block ++;
										strcpy(auxBuffer , "ATM> -3 : Pin gresit");
										send(i , auxBuffer , strlen(auxBuffer), 0);
										j++;
										break;
									}
								}else{
									j++;
								}

							}
							if(j == nrLines && auxBuffer[0] != 'A'){
							strcpy(auxBuffer , "ATM> -4 : Numar card inexistent");
							send(i , auxBuffer , strlen(auxBuffer), 0);
							}
						}else if(strncmp(buffer , "logout" , 6) == 0){
							int j = 0 ;
							while(j <nrLines ){
								if(clienti[j].active == 1 && clienti[j].sock == i){
									clienti[j].active = 0;
									clienti[j].sock = -1;
									break;
								}else{
									j++;
								}
							}
							strcpy(auxBuffer , "ATM> Deconectare de la bancomat");
							send(i , auxBuffer , strlen(auxBuffer), 0);
						}else if(strncmp(buffer, "listsold" , 8) == 0){
							int j = 0;
							memset(money , 0 , 50);
							while(j< nrLines){
								if(clienti[j].active ==1 && clienti[j].sock ==i){
									break;
								}else{
									j++;
								}
							}
							if(j< nrLines){
							strcpy(auxBuffer , "ATM> ");
							sprintf(money , "%.2lf" , clienti[j].sold);
							strcat(auxBuffer , money);
							send(i , auxBuffer , strlen(auxBuffer), 0);
							}else{
								strcpy(auxBuffer , "ATM> -1 : Clientul nu este autentificat ");
								send(i , auxBuffer , strlen(auxBuffer), 0);
							}
						}else if(strncmp(buffer ,"getmoney" , 8) == 0){
							int j = 0;
							auxMoney = 0;
							memset(money , 0 , 50);
							
							while(j< nrLines){
								if(clienti[j].active ==1 && clienti[j].sock == i){
									break;
								}else{
									j++;
								}
							}
							if(j< nrLines){
								token = strtok(buffer , " ");
								token = strtok(NULL , " ");
								strcpy(money , token);
								auxMoney = atoi(money);
								if(auxMoney % 10 != 0){
									strcpy(auxBuffer , "ATM> -9 : Suma nu este multiplu de 10");
									send(i , auxBuffer , strlen(auxBuffer), 0);
								}else if(clienti[j].sold - auxMoney < 0){
									strcpy(auxBuffer , "ATM> -8 : Fonduri insuficiente");
									send(i , auxBuffer , strlen(auxBuffer), 0);

								}else{
									clienti[j].sold = clienti[j].sold - auxMoney;
									strcpy(auxBuffer , "ATM> Suma ");
									strncat(auxBuffer , money ,strlen(money) -1);
									strcat(auxBuffer , " retrasa cu succes");
									send(i , auxBuffer , strlen(auxBuffer), 0);
								}

							}else{
								strcpy(auxBuffer , "ATM> -1 : Clientul nu este autentificat ");
								send(i , auxBuffer , strlen(auxBuffer), 0);
							}	
						}else if(strncmp(buffer , "putmoney" , 8) == 0){
							memset(money , 0 , 50);
							double auxMoney1 = 0 ;
							int j =0;

							while(j< nrLines){
								if(clienti[j].active ==1 && clienti[j].sock == i ){
									break;
								}else{
									j++;
								}
							}
							if(j< nrLines){
								token = strtok(buffer , " ");
								token = strtok(NULL , " ");
								strcpy(money , token);
								sscanf(money , "%lf" , &auxMoney1);
								clienti[j].sold = clienti[j].sold + auxMoney1;
								strcpy(auxBuffer , "ATM> Suma depusa cu succes");
								send(i , auxBuffer , strlen(auxBuffer), 0);
							}else{
								strcpy(auxBuffer , "ATM> -1 : Clientul nu este autentificat ");
								send(i , auxBuffer , strlen(auxBuffer), 0);
							}

						}else{
							strcpy(auxBuffer , "ATM> -11 : Comanda invalida ");
							send(i , auxBuffer , strlen(auxBuffer), 0);
						}	
					} 
				}
			}
     	}
    }
     close(sockfd);
     close(udpsock);
     fclose(source);
     return 0; 
}


