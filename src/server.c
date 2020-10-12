#include "server.h"

uint16_t currentBlockNum;

//Récupère les données
char* retrieveData(char* filename, char* data) {
    FILE* fptr;
    char* linebuffer;

    //On vérifie qu'on arrive à malloc
    if ((linebuffer = (char*) malloc(FILE_SIZE * sizeof(char))) == NULL) {
        perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");
        exit(-1);
    }

    //On vérifie qu'on peut ouvrir le fichier
    if ((fptr = fopen(filename, "r")) == NULL) {
        perror("\n===================\nServerEcho : No such file or directory\n===================\n\n");
        free(linebuffer);
        exit(-1);
    }

    //On récupère le contenu du fichier ligne par ligne
    while(fgets(linebuffer, FILE_SIZE, fptr) != NULL) {
        if ((strlen(data) + strlen(linebuffer) - 1)  >= strlen(data)) {        //Si data est trop petit, on l'agrandit
            data = (char*) realloc(data, strlen(data) + strlen(linebuffer));
        }

        strcat(data, linebuffer);
    }

    //printf("%s\n", data);

    free(linebuffer);

    fclose(fptr);
    return data;
}


//Créateur de segment
segment** createDataSegments(char* data) {
    //On définit le nombre de segment à envoyer
    size_t dataLen = strlen(data);
    int segNum = (dataLen / DATA_SIZE) + 1;

    segment** segments = NULL;
    char* littleData = NULL;
    uint16_t tempCurrentBlockNum = currentBlockNum;
    int startIndex = 0;
    int endIndex = 0;

    if ((segments = (segment**) malloc(segNum * sizeof(segment*))) == NULL) {
        perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");
        exit(-1);
    }

    for (int i = 0; i < segNum; i++) {
        if ((segments[i] = (segment*) malloc(sizeof(segment))) == NULL) {
            perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");
            exit(-1);
        }

        //On fait en sorte que le dernier segments ai une partie data < 512 & on récupère le bon bout de données
        if (i < segNum - 1){
            if ((littleData = (char*) malloc(DATA_SIZE * sizeof(char))) == NULL) {
                perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");
                exit(-1);
            }
            startIndex = i * DATA_SIZE;
            endIndex = (i+1) * DATA_SIZE;

            for (int j = startIndex; j < endIndex; j++)
                littleData[j - startIndex] = data[j];
        } else {
            int lastDataLength = dataLen - ((segNum - 1) * DATA_SIZE);
            startIndex = i * DATA_SIZE;

            if ((littleData = (char*) malloc(lastDataLength * sizeof(char) + 1)) == NULL) {
                perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");
                exit(-1);
            }

            for (size_t j = startIndex; j < dataLen; j++)
                littleData[j - startIndex] = data[j];

            littleData[dataLen - startIndex] = '\0';
        }

        //On met le type dans le segment
        segments[i]->type = 3;
        if (segments[i]->type != 3) {
            perror("\n===================\nServerEcho : Erreur de création de segment de données\n===================\n\n");

            for(int j = 0; j < i+1; j++)
                free(segments[j]);
            free(segments);
            free(littleData);

            exit(-1);
        }

        //On met le numéro de block
        segments[i]->blockNum = tempCurrentBlockNum;
        if (segments[i]->blockNum != tempCurrentBlockNum) {
            perror("\n===================\nServerEcho : Erreur de création de segment de données\n===================\n\n");

            for(int j = 0; j < i+1; j++)
                free(segments[j]);
            free(segments);
            free(littleData);

            exit(-1);
        }

        //On met la data
        segments[i]->data = littleData;
        if (segments[i]->data != littleData) {
            perror("\n===================\nServerEcho : Erreur de création de segment de données\n===================\n\n");
            for(int j = 0; j < i+1; j++)
                free(segments[j]);
            free(segments);
            free(littleData);
            exit(-1);
        }

        tempCurrentBlockNum++;

        //printf("\nType : %u\nNuméro de block : %u\nData : %s\n\n", (uint16_t) segments[i]->type, (uint16_t) segments[i]->blockNum, segments[i]->data);
    }

    //printf("Second : %p\n", segments);
    // for(int i = 0; i < segNum; i++)
    //     printf("\nType : %lu\nNuméro de block : %lu\nData : %s\nPointeur : %p\n\n", (uint16_t) segments[i]->type, (uint16_t) segments[i]->blockNum, segments[i]->data, segments);

    return segments;
}


//Passage de segment (struct) à string
char* segToData(segment* seg) {
    char* data = NULL;

    if ((data = (char*) malloc(sizeof(seg->type) + sizeof(seg->blockNum) + (strlen(seg->data) * sizeof(char)))) == NULL)
        return NULL;

    //On remplit data avec le segment à envoyer
    data[0] = (seg->type >> 8) + 48;
    data[1] = (seg->type) + 48;
    data[2] = (seg->blockNum >> 8) + 48;
    data[3] = (seg->blockNum) + 48;

    size_t i = 4;
    for(i = 0; i < strlen(seg->data); i++)
        data[i + 4] = seg->data[i];
    data[i + 4] = '\0';

    return data;
}

int main(int argc, char* argv[]){
    //Variables de configuration du serveur
    int clientSocket;
    size_t n;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    //Variables d'utilisation du serveur
    char rcvBuffer[MAX_BUFFER_SIZE];
    rcvSegment* rcvSeg;
    char* temp = NULL;
    char* data = NULL;
    segment** dataSegments = NULL;
    int segmentsIndexMax = 0;
    int segmentsIndexToSend = 0;
    uint16_t lastAckNum = 0;

    //Variables de gestion du timer
    struct timespec sendTime;
    struct timespec currentTime;
    time_t resendCooldown = BASE_RESENDCLD;
    time_t prevSendTime = 0;

    //On vérifie qu'il y a le bon nombre d'arguments
    if (argc < 3) {
        perror("\n===================\nServerEcho : Trop peu d'arguments\n===================\n\n");
        exit(-1);
    } else if (argc > 3) {
        perror("\n===================\nServerEcho : Trop d'arguments\n===================\n\n");
        exit(-1);
    }


    //Mettre l'adresse du client dans la structure serv_addr
    memset((char*) &client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(argv[1]);
    client_addr.sin_port = htons(atoi(argv[2]));

    //Création du socklen_t
    if ((clientSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("\n===================\nServerEcho : Erreur lors de la création du socket\n===================\n\n");
        exit(-1);
    }

    //Bind
    if (bind(clientSocket, (struct sockaddr*) &client_addr, sizeof(client_addr)) < 0) {
        perror("\n===================\nServerEcho : Erreur de bind\n===================\n\n");
        exit(-1);
    }

    currentBlockNum = 1;

    for (;;) {
        //Reçu de
        if ((n = recvfrom(clientSocket, rcvBuffer, sizeof(rcvBuffer)-1, 0, (struct sockaddr*) &client_addr, &len)) >= MAX_BUFFER_SIZE) {
            perror("\n===================\nServerEcho : Erreur recvfrom - segment trop volumineux\n===================\n\n");
            exit(-1);
        }


        if (n < MIN_BUFFER_SIZE) {
            perror("\n===================\nServerEcho : Erreur recvfrom - segment trop pitit\n===================\n\n");
            exit(-1);
        }

        rcvBuffer[n] = '\0';

        //On formate rcvBuffer reçue en une structure rcvSegment
        if ((rcvSeg = (rcvSegment*) malloc(sizeof(rcvSegment))) == NULL) {
            perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");
            exit(-1);
        }

        rcvSeg->type = (uint16_t) (rcvBuffer[0] - 48) << 8;
        rcvSeg->type = (uint16_t) (rcvBuffer[1] - 48);
        rcvSeg->blockNum = -1;
        rcvSeg->filename = NULL;
        rcvSeg->mode = NULL;

        //On regarde si le seg n'est pas un ACK

        if ((temp = (char*) malloc(2*sizeof(char))) == NULL) {
            perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");

            free(rcvSeg->filename);
            free(rcvSeg->mode);
            free(rcvSeg);

            exit(-1);
        }

        if (strlen(rcvBuffer) >= 6){
            printf("\nDemande de lecture reçue\n\n");
            //On s'occupe du filename
            if ((rcvSeg->filename = (char*) malloc(DATA_SIZE * sizeof(char))) == NULL) {
                perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");

                free(rcvSeg->filename);
                free(rcvSeg->mode);
                free(rcvSeg);
                free(temp);

                exit(-1);
            }

            int i = 2;
            temp[1] = '\0';
            while (rcvBuffer[i] != '0'){
                temp[0] = rcvBuffer[i];
                rcvSeg->filename = strcat(rcvSeg->filename, temp);
                i++;
            }
            rcvSeg->filename[i - 2] = '\0';
            printf("\nFilename : %s\n\n", rcvSeg->filename);

            //on s'occupe du mode
            if ((rcvSeg->mode = (char*) malloc(6 * sizeof(char))) == NULL) {
                perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");

                free(rcvSeg->filename);
                free(rcvSeg->mode);
                free(rcvSeg);
                free(temp);

                exit(-1);
            }

            strcpy(rcvSeg->mode, "octet");

        } else {
            temp[0]=rcvBuffer[2];
            temp[1]=rcvBuffer[3];
            rcvSeg->blockNum = (uint16_t) atoi((char*) temp);
        }
        free(temp);

        switch (rcvSeg->type) {
            case (uint16_t) 1:     //RRQ
                //On vérifie qu'on arrive bien à malloc
                if ((data = (char*) malloc(FILE_SIZE * sizeof(char))) == NULL) {
                    perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");

                    free(rcvSeg->filename);
                    free(rcvSeg->mode);
                    free(rcvSeg);

                    exit(-1);
                }

                //On récupère la donnée à envoyer
                data = retrieveData(rcvSeg->filename, data);
                dataSegments = createDataSegments(data);


                free(data);

                //Envoie du premier segments
                segmentsIndexToSend = 0;
                segmentsIndexMax = (strlen(data) / DATA_SIZE) + 1;

                //La String data deviens maintenant la chaîne de caractères du seg à envoyer
                if ((data = segToData(dataSegments[segmentsIndexToSend])) == NULL) {
                    perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");

                    free(rcvSeg->filename);
                    free(rcvSeg->mode);
                    free(rcvSeg);
                    for(int i = 0; i < segmentsIndexMax; i++)
                        free(dataSegments[i]);
                    free(dataSegments);

                    exit(-1);
                }


                if ((n = sendto(clientSocket, (char *) data, strlen(data), 0, (struct sockaddr*) &client_addr, sizeof(client_addr))) != strlen(data)) {
                    perror("\n===================\nServerEcho : Erreur sendto\n===================\n\n");

                    free(rcvSeg->filename);
                    free(rcvSeg->mode);
                    free(rcvSeg);
                    for(int i = 0; i < segmentsIndexMax; i++)
                        free(dataSegments[i]);
                    free(dataSegments);

                    exit(-1);
                }
                printf("Premier segment envoyé\n");

                free(data);

                timespec_get(&sendTime, TIME_UTC);
                segmentsIndexToSend++;
                break;

            case 2: //WRQ
            case 3: //Data
                break;

            case 4: //ack
                lastAckNum = rcvSeg->blockNum;
                //Si on reçoit le bon acquitement, on envoie le segment suivant
                if(lastAckNum == currentBlockNum) {
                    printf("Acquittement %d reçu \n\n", currentBlockNum);
                    data = (char*) realloc(data, 516 * sizeof(char));
                    //On envoie le segment suivant
                    //La String data deviens maintenant la chaîne de caractères du seg à envoyer
                    if ((data = segToData(dataSegments[segmentsIndexToSend])) == NULL) {
                        perror("\n===================\nServerEcho : Erreur d'allocation de la mémoire\n===================\n\n");

                        free(rcvSeg->filename);
                        free(rcvSeg->mode);
                        free(rcvSeg);
                        for(int i = 0; i < segmentsIndexMax; i++)
                            free(dataSegments[i]);
                        free(dataSegments);

                        exit(-1);
                    }



                    if ((n = sendto(clientSocket, data, strlen(data), 0, (struct sockaddr*) &client_addr, sizeof(client_addr))) != strlen(data)) {
                        perror("\n===================\nServerEcho : Erreur sendto\n===================\n\n");

                        free(rcvSeg->filename);
                        free(rcvSeg->mode);
                        free(rcvSeg);
                        for(int i = 0; i < segmentsIndexMax; i++)
                            free(dataSegments[i]);
                        free(dataSegments);

                        exit(-1);
                    }
                    printf("Segment %d envoyé\n", currentBlockNum+1);

                    prevSendTime = sendTime.tv_sec;
                    timespec_get(&sendTime, TIME_UTC);
                    resendCooldown = (time_t) (sendTime.tv_sec + prevSendTime) / 2;

                    segmentsIndexToSend++;
                    currentBlockNum++;

                    free(data);
                } else {
                    perror("\n===================\nServerEcho : Erreur acknumber\n===================\n\n");

                    free(rcvSeg->filename);
                    free(rcvSeg->mode);
                    free(rcvSeg);
                    for(int i = 0; i < segmentsIndexMax; i++)
                        free(dataSegments[i]);
                    free(dataSegments);

                    exit(-1);
                }

                break;
        }

        //Si on ne reçoit pas d'ACK c'est qu'un seg s'est perdu
        timespec_get(&currentTime, TIME_UTC);
        if ((currentTime.tv_sec - sendTime.tv_sec) >= resendCooldown) {
            if ((n = sendto(clientSocket, dataSegments[segmentsIndexToSend-1], sizeof(dataSegments[segmentsIndexToSend-1]), 0, (struct sockaddr*) &client_addr, sizeof(client_addr))) != sizeof(dataSegments[segmentsIndexToSend-1])) {
                perror("ServerEcho : Erreur sendto\n");

                free(rcvSeg->filename);
                free(rcvSeg->mode);
                free(rcvSeg);
                for(int i = 0; i < segmentsIndexMax; i++)
                    free(dataSegments[i]);
                free(dataSegments);

                exit(-1);
            }
        }

        //Free c'est la vie
        if (rcvSeg) {
            free(rcvSeg->filename);
            free(rcvSeg->mode);
            free(rcvSeg);
        }
        if (segmentsIndexMax != 0 && (segmentsIndexToSend == segmentsIndexMax) && (lastAckNum == currentBlockNum)) {
            for(int i = 0; i < segmentsIndexMax; i++)
                free(dataSegments[i]);
            free(dataSegments);

            printf("C'est terminé!\n");//comment aller ici??
            return 0;
        }
      }
}
