#include "client.h"

int acknumber = 0;


char *create_request(char *filename,char *mode){
  //01 correspond a RRQ (on se concentre sur ça dans ce projet, WRQ est facultatif)
  //mode = octet ici d'après le sujet
  //01 | filename | 0 | mode | 0 -> format du paquet de demande de connexion/d'autorisation de lecture
  size_t filename_length = strlen(filename);
  char *request = NULL;

  if ((request = (char*) malloc((filename_length + 10) * sizeof(char))) == NULL){
    perror("\n===================\nClientEcho : Erreur d'allocation de la mémoire\n===================\n\n");
    exit(EXIT_FAILURE);
  }

  strcpy(request, "01");
  //request = strcat(request,"01");
  request = strcat(request,filename);
  request = strcat(request,"0");
  request = strcat(request,mode);
  request = strcat(request,"0");

  printf("request : %s\n", request);

  return request;
}

char *create_ack(int acknumber){
  char *ack;
  if ((ack = (char*)malloc(4*sizeof(char))) == NULL){
    perror("\n===================\nClientEcho : Erreur d'allocation de la mémoire\n===================\n\n");
    exit(EXIT_FAILURE);
  }
  ack = strcat(ack,"04");
  char *an = (char*)malloc(2*sizeof(char));
  if (acknumber<10){
    sprintf(an,"0%d",acknumber);
  }else{
    sprintf(an,"%d",acknumber);
  }
  ack = strcat(ack,an);
  free(an);
  return ack;
}

int main(int argc, char *argv[]) {
  char *filename = argv[3];
  FILE* fichier = fopen("output.txt","a");

  //Variables de gestion du timer
  struct timespec sendTime;
  struct timespec currentTime;
  time_t resendCooldown = BASE_RESENDCLD;
  time_t prevSendTime = 0;

  //On vérifie qu'il y a le bon nombre d'arguments
  if (argc < 4) {
      perror("\n===================\nServerEcho : Trop peu d'arguments\n===================\n\n");
      exit(-1);
  } else if (argc > 4) {
      perror("\n===================\nServerEcho : Trop d'arguments\n===================\n\n");
      exit(-1);
  }

  //Mise en place de la connexion UDP
	int sockfd;
	struct sockaddr_in servaddr;
    socklen_t len = sizeof(servaddr);

	// Création descripteur de socket
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
		perror("\n===================\nClientEcho : Erreur lors de la création du socket\n===================\n\n");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET; //UDP
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    //Partie communication

    //création et envoi de la demande de connexion
    char *request = create_request(filename,"octet");
    ssize_t n;
    if ((n = sendto(sockfd,(const char*)request,strlen(request),0,(const struct sockaddr *)&servaddr,len))<=9){
        perror("\n===================\nClientEcho : Erreur sendto\n===================\n\n");
        exit(EXIT_FAILURE);
    }
    printf("\nDemande de lecture envoyée\n\n");
    timespec_get(&sendTime, TIME_UTC);

    //Réception des données
    //char* reception = (char*) malloc(516*sizeof(char));

    for(;;){
        char *reception=(char*)calloc(516,sizeof(char));
        ssize_t received = recvfrom(sockfd,reception,516,0,(struct sockaddr *)&servaddr,&len);

        if (received < MIN_BUFFER_SIZE){
          perror("\n===================\nClientEcho : Erreur recvfrom - segment trop pitit\n===================\n\n");
          exit(EXIT_FAILURE);

        }else if(received > MAX_BUFFER_SIZE){
          perror("\n===================\nClientEcho : Erreur recvfrom - segment trop volumineux\n===================\n\n");
          exit(EXIT_FAILURE);
        }
        printf("Paquet %d reçu! \n", acknumber+1);
        segment *rcvSeg;
        if ((rcvSeg = (segment *)malloc(sizeof(segment))) == NULL){
          perror("\n===================\nClientEcho : Erreur d'allocation de la mémoire\n===================\n\n");
          exit(EXIT_FAILURE);
        }

        rcvSeg->data = (char*) calloc(516,sizeof(char));

        char temp[2];
        temp[1]='\0';
        size_t i;
        for (i = 4;i < strlen(reception); i++){
          temp[0]=reception[i];
          rcvSeg->data = strcat(rcvSeg->data,temp);
        }

        fputs((const char*)rcvSeg->data, fichier);

        char *ack = (char*)malloc(4*sizeof(char));
        ack = create_ack(++acknumber);
        if ((n = sendto(sockfd,(const char*)ack,strlen(ack),0,(const struct sockaddr *)&servaddr,len))!=4){
          perror("\n===================\nClientEcho : Erreur sendto\n===================\n\n");
          exit(EXIT_FAILURE);
        }
        free(ack);
        printf("Ack %d envoyé\n\n", acknumber);
        prevSendTime = sendTime.tv_sec;
        timespec_get(&sendTime, TIME_UTC);
        resendCooldown = (time_t) (sendTime.tv_sec + prevSendTime) / 2;

        if(i<512){
          printf("C'est terminé!\n");
          break;
        }

        //Si on ne reçoit pas de nouveau segment, c'est que l'ack s'est perdu
        timespec_get(&currentTime, TIME_UTC);
        if ((currentTime.tv_sec - sendTime.tv_sec) >= resendCooldown) {
            if ((n = sendto(sockfd,(const char*)ack,strlen(ack),0,(const struct sockaddr *)&servaddr,len))!=4) {
                perror("ClientEcho : Erreur sendto\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    //I want to break freeeeeeeeeeee
    free(request);

    //Fermeture de connexion
    close(sockfd);
    fclose(fichier);
    return 0;
}
