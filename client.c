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
#include <pthread.h>

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

int v = 2;

/* Contadors per als temporitzadors de subscripcio */
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

struct socketHELLO {
    int sock;
    struct sockaddr_in Direccio;
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
    } else if(estat_actual == SUBS_ACK){
        printf("No s'ha pogut contactar amb el servidor.");
        exit(-3);
    }
}

void setTimeout(int milliseconds) {

    int milliseconds_since = clock() * 1000 / CLOCKS_PER_SEC;
    int end = milliseconds_since + milliseconds;

    if (milliseconds <= 0) {
        fprintf(stderr, "Count milliseconds for timeout is less or equal to 0\n");
        return;
    }

    /* wait while until needed time comes*/
    do {
        milliseconds_since = clock() * 1000 / CLOCKS_PER_SEC;
    } while (milliseconds_since <= end);
}

void *thread_hello(struct socketHELLO *args) {
    /* Thread independent per anar enviant HELLO's cada v segons */
    int a;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct socketHELLO *socketHELLO2 = (struct socketHELLO *) args;
    struct PDU_udp packetHELLO;
    struct PDU_udp respostaUDP;
    struct hostent *Host;
    struct sockaddr_in Direccion;
    /* int sock = socket(AF_INET, SOCK_DGRAM, 0); */
    /* Paquet HELLO UDP */
    packetHELLO.type = HELLO;
    strcpy(packetHELLO.mac, "12344566");
    strcpy(packetHELLO.aleatori, "00000000");
    strcpy(packetHELLO.dades, "");

    Host = gethostbyname("localhost");

    if (Host == NULL) {
        printf("Error\n");
    }
    memset(&Direccion, 0, sizeof(struct sockaddr_in));
    Direccion.sin_family = AF_INET;
    Direccion.sin_addr.s_addr = (((struct in_addr *) Host->h_addr)->s_addr);
    Direccion.sin_port = htons(2345);

    while (1) {
        printf("Enviar packet HELLO%i\n", socketHELLO2->sock);
        /* Paquet HELLO amb el servidor */
        a = sendto(sock,
                   &packetHELLO,
                   sizeof(packetHELLO),
                   0,
                   (struct sockaddr *) &Direccion,
                   sizeof(Direccion));
        if (a < 0) {
            fprintf(stderr, "Error al sendto\n");
            perror("Error ");
            exit(-2);
        }
        /* RECV del paquet HELLO i
         * posterior comprobacio */
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

        setTimeout(v * 1000);
    }
    return 0;
}

void lectura_configuracio() {
    FILE *fp;
    char line_separator;
    char key[20];
    char value[50];

    memset(&clientC, 0, sizeof(struct client));

    fp = fopen(fitxer_conf, "r");
    if (fp == NULL) {
        perror("ERROR ");
        exit(-1);
    }

    /* Name */
    fscanf(fp, "%s %c %s", key, &line_separator, value);
    strcpy(clientC.id, value);

    /* Situation */
    fscanf(fp, "%s %c %s", key, &line_separator, value);
    strcpy(clientC.situation, value);

    /* Elements */
    fscanf(fp, "%s %c %s", key, &line_separator, value);
    strcpy(clientC.elements, value);

    /* MAC */
    fscanf(fp, "%s %c %s", key, &line_separator, value);
    strcpy(clientC.mac, value);

    /* Local-TCP */
    fscanf(fp, "%s %c %d", key, &line_separator, &clientC.portTCP);

    /* Server IP */
    fscanf(fp, "%s %c %s", key, &line_separator, value);
    strcpy(clientC.ip, value);

    /* Server UDP port */
    fscanf(fp, "%s %c %d", key, &line_separator, &clientC.portUDP);


    printf("Client %s\n", clientC.id);
    printf("Situation %s\n", clientC.situation);
    printf("Elements %s\n", clientC.elements);
    printf("MAC %s\n", clientC.mac);
    printf("Local TCP Port: %i\n", clientC.portTCP);
    printf("Server ip: %s\n", clientC.ip);
    printf("Server upd port: %i\n", clientC.portUDP);

    fclose(fp);
}

void wait_ack_subscripcio(int sock, struct sockaddr_in Direccio) {
    int a;
    pthread_t Hilo;
    struct PDU_udp respostaUDP;
    struct socketHELLO *socketHELLO1;


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
    if (respostaUDP.type == SUBS_ACK) {
        printf("SUBS_ACK\n");
        estat_actual = SUBSCRIBED;
        /* Comença a enviar HELLO */
        /*memset(&socketHELLO1, 0, sizeof(socketHELLO1));*/
        socketHELLO1 = (struct socketHELLO *) malloc(sizeof(socketHELLO1));
        socketHELLO1->sock = sock;
        socketHELLO1->Direccio = Direccio;

        if (pthread_create(&Hilo, NULL, (void *(*)(void *)) thread_hello, (void *) &socketHELLO1)) {
            perror("ERROR creating thread.");
        }
        pthread_join(Hilo, NULL);
    }
}

void subscripcio() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct hostent *Host;
    int Puerto = clientC.portUDP;
    struct sockaddr_in Direccion;
    int a;
    /* char dadcli[LONGDADES]; */
    struct PDU_udp enviamentUDP;
    /* struct PDU_udp respostaUDP; */
    char dadesEnviamentUDP[80];


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
    strcpy(enviamentUDP.mac, clientC.mac);
    strcpy(enviamentUDP.aleatori, "00000000");
    /* Preparació dades */
    strcpy(dadesEnviamentUDP, clientC.id);
    strcat(dadesEnviamentUDP, ",");
    strcat(dadesEnviamentUDP, clientC.elements);
    /* Enviament dades */
    strcpy(enviamentUDP.dades, dadesEnviamentUDP);

    /* Paquet iniciar de sincronitzacio amb el servidor */
    a = sendto(sock, &enviamentUDP, sizeof(enviamentUDP), 0, (struct sockaddr *) &Direccion, sizeof(Direccion));
    if (a < 0) {
        fprintf(stderr, "Error al sendto\n");
        perror("Error ");
        exit(-2);
    }
    printf("Enviament: %i\n", enviamentUDP.type);

    /* Canvi estat a WAIT_ACK_SUBS inicialitzem els
     * temporitzadors i esperem fins a que rebem un paquet de resposta*/
    estat_actual = WAIT_ACK_SUBS;
    alarm(t);
    wait_ack_subscripcio(sock, Direccion);
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

void print_stats() {
    printf("Client Stats\n");
    printf("-------------\n");
    printf("MAC: %s\n", clientC.mac);
    printf("Nom: %s\n", clientC.id);
    printf("Situacio: %s\n", clientC.situation);
}

void *lectura_consola() {
    char c[20] = "";
    while (strcmp(c, "quit") != 0) {
        scanf("%s", c);
        printf("Command entered: %s\n", c);
        /* En cas que la comanda sigui stats */
        if (strcmp(c, "stats") == 0) {
            print_stats();
        }
    }
    /* Comanda = quit */
    printf("EXIT !!!!\n");
    return 0;
}

int main(int argc, char **argv) {
    pthread_t lectura;
    /* Registre inicial de la alarma per al timer */
    signal(SIGALRM, signalarm);

    parsejar_arguments(argc, argv);
    lectura_configuracio();
    if (pthread_create(&lectura, NULL, &lectura_consola, NULL)) {
        perror("ERROR creating thread.");
    }

    subscripcio();
    return 0;
}
