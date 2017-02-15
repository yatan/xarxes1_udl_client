#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
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


char fitxer_conf[30] = "client.cfg";

struct client
{
    char id[8];
    char situation[12];
    char mac[12];
    char ip[15];
    int portTCP;
    int portUDP;
};

struct element
{
    char magnitud[3];
    char ordinal[1];
    char tipus[1];
};
struct element* elements;

/*Estructura de dades TCP*/
struct PDU_tcp
{
    unsigned char type;
    char mac[7];
    char aleatori[13];
    char dispositiu[8];
    char valor[7];
    char info[80];
};

int main(int argc, char **argv)
{
    FILE *fp;
    char line_buffer[BUFSIZ];

    char identificador[8];
    char situation[12];
    char elements[7];
    char MAC[12];
    int local_tcp;
    char server_ip[15];
    int server_udp;



    fp = fopen(fitxer_conf,"r");
    if(fp == NULL)
    {
        printf("No es pot obrir el fitxer per registrar dades\n");
    }


    /* Name */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(identificador,strtok(NULL, " "), 9);


    /* Situation */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(situation,strtok(NULL, " "), 13);


    /* Elements */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(elements,strtok(NULL, " "), 8);

    /* MAC */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    strxfrm(MAC,strtok(NULL, " "), 13);

    /* Local-TCP */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    local_tcp = atoi(strtok(NULL, " "));

    /* Server IP */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    server_ip[strlen(server_ip) - 1] = '\0';
    /* Al tenir direccions on no es posen tots els numeros, el valor es variable */
    strxfrm(server_ip,strtok(NULL, " "), 16);


    /* Server UDP port */
    fgets(line_buffer, sizeof(line_buffer), fp);
    strtok(line_buffer, " ");
    server_udp = atoi(strtok(NULL, " "));


    printf("Client %s\n", identificador);
    printf("Situation %s\n", situation);
    printf("Elements %s\n", elements);
    printf("MAC %s\n", MAC);
    printf("Local TCP Port: %i\n", local_tcp);
    printf("Server ip: %s\n", server_ip);
    printf("Server upd port: %i\n", server_udp);


    fclose(fp);
    return 0;
}
