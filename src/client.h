/* Principe TFTP :
1) Demande d'autorisation de lecture (=demande de connection) -> request RRQ
2) Une fois que le serveur a ouvert la connection, le fichier est envoyé par paquets de 512 octets.
3) On doit attendre que chaque paquet soit ACK avant d'envoyer le suivant -> un ack contient le numéro de block envoyé (1 pour le premier bloc, et +1 à chaque bloc ensuite)
4) Un paquet de moins de 512 octets marque la fin de la transmission
5) Mise en place d'un timeout pour la gestion des paquets perdus. Une fois dépassé -> renvoie du dernier paquet ou ACK attendu
6) Ainsi chaque parti doit garder uniquement un paquet "en main" à a fois : dès qu'il a reçu la confirmation de la réception, il peut s'en débarasser et passer au suivant

A chaque réception d'un truc, il faut vérifier que le TID correspond à l'un des TID communiqué lors des 2 premières étapes (parce que sinon ça veut dire que l'envoie vient d'ailleurs)
Pour les erreurs on verra après*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>

#define MAX_BUFFER_SIZE 516
#define MIN_BUFFER_SIZE 4
#define BASE_RESENDCLD 5

typedef struct segment{
    uint16_t type;
    uint16_t blockNum;
    char* data;
} segment;

typedef struct requestSegment{
    uint16_t type;
    uint16_t blockNum;
    char* filename;
    char* mode;
} requestSegment;


char *create_request(char *filename,char *mode);
char *create_ack(int acknumber);
int main(int argc, char *argv[]);
