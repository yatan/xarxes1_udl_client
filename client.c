#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
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

/* Global variables */
int debug = 0;
char fitxer_conf[30] = "client.cfg";
unsigned char estat_actual = NOT_SUBSCRIBED;

/* Control temporitzacio subscripcio */

int t = 1;
int u = 3;
int n = 8;
int o = 3;

int u_cont = 0;
int n_cont = 0;
int o_cont = 0;


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

/*Estructura de dades UDP*/
struct PDU_udp {
    unsigned char type;
    char mac[13];
    char aleatori[9];
    char dades[80];
};

void signalarm(int sig) {
    signal(SIGALRM, SIG_IGN);          /* ignore this signal       */
    printf("Alarm !! o:%i n:%i \n", o_cont, n_cont);
    signal(SIGALRM, signalarm);     /* reinstall the handler    */

    if (o_cont < o) {
        if (estat_actual == WAIT_ACK_SUBS && n_cont < n) {
            /* envia pack subs */
            n_cont++;
            alarm(t);
        } else if (estat_actual == WAIT_ACK_SUBS && n_cont == n) {
            n_cont++;
            alarm(u);
        } else {
            o_cont++;
            n_cont = 0;
            alarm(t);
        }
    } else {
        printf("No s'ha pogut contactar amb el servidor.");
        exit(-3);
    }

}


void lectura_configuracio() {
    FILE *fp;
    char line_buffer[50];

    memset(&clientC, 0, sizeof(struct client));

    fp = fopen(fitxer_conf, "r");
    if (fp == NULL) {
        perror("ERROR ");
        exit(-1);
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

void wait_ack_subscripcio(int sock) {
    int a;
    struct PDU_udp respostaUDP;

    /* Paquet de resposta amb la confirmacio del servidor */
    memset(&respostaUDP, 0, sizeof(respostaUDP));
    a = recvfrom(sock, &respostaUDP, sizeof(respostaUDP), 0, (struct sockaddr *) 0, (int *) 0);
    if (a < 0) {
        fprintf(stderr, "Error al recvfrom\n");
        perror("Error ");
        exit(-2);
    }
    /* dadcli[a] = '\0'; */
    printf("Resposta: %i\n", respostaUDP.type);

    /*
     * Verificar el packet rebut
     */

    estat_actual = SUBSCRIBED;
}

void subscripcio() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct hostent *Host;
    int Puerto = 6667;
    struct sockaddr_in Direccion;
    int a;
    /* char dadcli[LONGDADES]; */
    struct PDU_udp enviamentUDP;
    /* struct PDU_udp respostaUDP; */

    Host = gethostbyname("localhost");

    if (Host == NULL) {
        printf("Error\n");
    }

    /* Dades connexio socket */
    memset(&Direccion, 0, sizeof(struct sockaddr_in));
    Direccion.sin_family = AF_INET;
    Direccion.sin_addr.s_addr = (((struct in_addr *) Host->h_addr)->s_addr);
    Direccion.sin_port = htons(Puerto);


    /* Paquet PDU SUBS_REQ */
    enviamentUDP.type = SUBS_REQ;
    strcpy(enviamentUDP.mac, "12344566");
    strcpy(enviamentUDP.aleatori, "00000000");
    strcpy(enviamentUDP.dades, "");

    /* Paquet iniciar de sincronitzacio amb el servidor */
    a = sendto(sock, &enviamentUDP, sizeof(enviamentUDP), 0, (struct sockaddr *) &Direccion, sizeof(Direccion));
    if (a < 0) {
        fprintf(stderr, "Error al sendto\n");
        perror("Error ");
        exit(-2);
    }
    printf("Enviament: %i\n", enviamentUDP.type);

    /* Canvi estat a WAIT_ACK_SUBS */
    estat_actual = WAIT_ACK_SUBS;
    alarm(t);
    wait_ack_subscripcio(sock);

}

void parsejar_arguments(int argc, char **argv) {
    int index;
    int c;

    while ((c = getopt(argc, argv, "dc:")) != -1)
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 'c':
                strcpy(fitxer_conf, optarg);
                break;
            case '?':
                if (optopt == 'c') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                    exit(-1);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                    exit(-1);
                } else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
            default:
                abort();
        }

    printf("debug = %d, cvalue = %s\n",
           debug, fitxer_conf);

    for (index = optind; index < argc; index++)
        printf("Non-option argument %s\n", argv[index]);
}

int main(int argc, char **argv) {
    /* Registre inicial de la alarma per al timer */
    signal(SIGALRM, signalarm);

    parsejar_arguments(argc, argv);
    lectura_configuracio();
    subscripcio();
    return 0;
}
