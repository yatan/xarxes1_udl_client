#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/mman.h>

/* Tipus paquet subscripcio */
#define SUBS_REQ 0x00
#define SUBS_ACK 0x01
#define SUBS_INFO 0x02
#define INFO_ACK 0x03
#define SUBS_NAK 0x04
#define SUBS_REJ 0x05

/* Estats en subscripcio */
#define DISCONNECTED 0xa0
#define NOT_SUBSCRIBED 0xa1
#define WAIT_ACK_SUBS 0xa2
#define WAIT_ACK_INFO 0xa3
#define SUBSCRIBED 0xa4
#define SEND_HELLO 0xa5
#define WAIT_INFO 0xa6

/* Tipus paquet comunicacio periodica */
#define HELLO 0x10
#define HELLO_REJ 0x11

/* Tipus paquet PDU */
#define SEND_DATA 0x20
#define SET_DATA  0x21
#define GET_DATA  0x22
#define DATA_ACK  0x23
#define DATA_NACK  0x24
#define DATA_REJ  0x25

#define LONGDADES 100

char fitxer_conf[30] = "client.cfg";

/* Variables del client obtinguts a partir
 * del fitxer de configuracio */
struct client {
    char id[9];
    char situation[13];
    char elements[8];
    char mac[13];
    char ip[16];
    int portTCP;
    int portUDP;
} clientC;


struct element {
    char magnitud[3];
    char ordinal[1];
    char tipus[1];
};
struct element *elements;

/*Estructura de dades TCP*/
struct PDU_tcp {
    unsigned char type;
    char mac[7];
    char aleatori[13];
    char dispositiu[8];
    char valor[7];
    char info[80];
};

void lectura_configuracio() {
    FILE *fp;
    char line_buffer[50];

    memset(&clientC, 0, sizeof(struct client));

    fp = fopen(fitxer_conf, "r");
    if (fp == NULL) {
        printf("No es pot obrir el fitxer per registrar dades\n");
    }


    /* Name */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(clientC.id, strtok(NULL, " "), 9);


    /* Situation */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(clientC.situation, strtok(NULL, " "), 13);


    /* Elements */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(clientC.elements, strtok(NULL, " "), 8);

    /* MAC */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(clientC.mac, strtok(NULL, " "), 13);

    /* Local-TCP */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    clientC.portTCP = atoi(strtok(NULL, " "));

    /* Server IP */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(clientC.ip, strtok(NULL, " "), 16);

    /* Server UDP port */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    clientC.portUDP = atoi(strtok(NULL, " "));


    printf("Client %s\n", clientC.id);
    printf("Situation %s\n", clientC.situation);
    printf("Elements %s\n", clientC.elements);
    printf("MAC %s\n", clientC.mac);
    printf("Local TCP Port: %i\n", clientC.portTCP);
    printf("Server ip: %s\n", clientC.ip);
    printf("Server upd port: %i\n", clientC.portUDP);

    fclose(fp);
}

void subscripcio() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct hostent *Host;
    int Puerto = 6667;

    Host = gethostbyname("localhost");
    printf("%s", Host->h_name);

    if (Host == NULL) {
        printf("Error\n");
    }

    struct sockaddr_in Direccion;
    memset(&Direccion, 0, sizeof(struct sockaddr_in));
    Direccion.sin_family = AF_INET;
    Direccion.sin_addr.s_addr = (((struct in_addr *) Host->h_addr)->s_addr);
    Direccion.sin_port = htons(Puerto);


    int a;
    char dadcli[LONGDADES];
    /* Paquet per disparar la resposta amb el temps. */
    strcpy(dadcli, "DUMMY");
    a = sendto(sock, dadcli, strlen(dadcli) + 1, 0, (struct sockaddr *) &Direccion, sizeof(Direccion));
    if (a < 0) {
        fprintf(stderr, "Error al sendto\n");
        perror("Error ");
        exit(-2);
    }

}

int main(int argc, char **argv) {
    //lectura_configuracio();
    subscripcio();
    return 0;
}
