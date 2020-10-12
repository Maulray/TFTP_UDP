#ifndef DEF_SERVER
#define DEF_SERVER

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <time.h>

#define DATA_SIZE 512
#define MAX_BUFFER_SIZE 516
#define MIN_BUFFER_SIZE 4
#define FILE_SIZE 2048
#define BASE_RESENDCLD 5

typedef struct segment{
    uint16_t type;
    uint16_t blockNum;
    char* data;
} segment;

//Dans ce que je récupère de ce que tu m'envoies, j'ai pas besoin de l'octet à 0 après le filename (c'est pour ça qu'il est pas dans la structure)
//Mais j'ai quand même besoin que tu me l'envoies puisqu'il me sert à trouver la fin du filename (cf server.c ligne 183) <3
typedef struct rcvSegment{
    uint16_t type;
    uint16_t blockNum;
    char* filename;
    char* mode;
} rcvSegment;

char* retrieveData(char* filename, char* data);
segment** createDataSegments(char* data);
char* segToData(segment* seg);
int main(int argc, char* argv[]);

#endif
