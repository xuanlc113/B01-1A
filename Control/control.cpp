#include <ncurses.h>
// Routines to create a TLS client
#include "client_lib/make_tls_client.h"

// Network packet types
#include "client_lib/netconstants.h"

// Packet types, error codes, etc.
#include "client_lib/constants.h"

// Tells us that the network is running.
static volatile int networkActive = 0;

void handleError(const char *buffer) {
    switch (buffer[1]) {
        case RESP_OK:
            printf("Command / Status OK\n");
            break;

        case RESP_BAD_PACKET:
            printf("BAD MAGIC NUMBER FROM ARDUINO\n");
            break;

        case RESP_BAD_CHECKSUM:
            printf("BAD CHECKSUM FROM ARDUINO\n");
            break;

        case RESP_BAD_COMMAND:
            printf("PI SENT BAD COMMAND TO ARDUINO\n");
            break;

        case RESP_BAD_RESPONSE:
            printf("PI GOT BAD RESPONSE FROM ARDUINO\n");
            break;

        default:
            printf("PI IS CONFUSED!\n");
    }
}

void handleStatus(const char *buffer) {
    int32_t data[16];
    memcpy(data, &buffer[1], sizeof(data));

    printf("\n ------- ALEX STATUS REPORT ------- \n\n");
    printf("Left Forward Ticks:\t\t%d\n", data[0]);
    printf("Right Forward Ticks:\t\t%d\n", data[1]);
    printf("Left Reverse Ticks:\t\t%d\n", data[2]);
    printf("Right Reverse Ticks:\t\t%d\n", data[3]);
    printf("Left Forward Ticks Turns:\t%d\n", data[4]);
    printf("Right Forward Ticks Turns:\t%d\n", data[5]);
    printf("Left Reverse Ticks Turns:\t%d\n", data[6]);
    printf("Right Reverse Ticks Turns:\t%d\n", data[7]);
    printf("Forward Distance:\t\t%d\n", data[8]);
    printf("Reverse Distance:\t\t%d\n", data[9]);
    printf("\n---------------------------------------\n\n");
}

void handleMessage(const char *buffer) { printf("MESSAGE FROM ALEX: %s\n", &buffer[1]); }

void handleCommand(const char *buffer) {
    // We don't do anything because we issue commands
    // but we don't get them. Put this here
    // for future expansion
}

void handleNetwork(const char *buffer, int len) {
    // The first byte is the packet type
    int type = buffer[0];

    switch (type) {
        case NET_ERROR_PACKET:
            handleError(buffer);
            break;

        case NET_STATUS_PACKET:
            handleStatus(buffer);
            break;

        case NET_MESSAGE_PACKET:
            handleMessage(buffer);
            break;

        case NET_COMMAND_PACKET:
            handleCommand(buffer);
            break;
    }
}

void sendData(void *conn, const char *buffer, int len) {
    int c;
    // printf("\nSENDING %d BYTES DATA\n\n", len);
    if (networkActive) {
        /* TODO: Insert SSL write here to write buffer to network */
        c = sslWrite(conn, buffer, len);
        /* END TODO */
        networkActive = (c > 0);
    }
}

void *readerThread(void *conn) {
    char buffer[128];
    int len;

    while (networkActive) {
        /* TODO: Insert SSL read here into buffer */
        len = sslRead(conn, buffer, sizeof(buffer));
        printf("read %d bytes from server.\n", len);

        /* END TODO */

        networkActive = (len > 0);

        if (networkActive) handleNetwork(buffer, len);
    }

    printf("Exiting network listener thread\n");

    /* TODO: Stop the client loop and call EXIT_THREAD */
    stopClient();
    EXIT_THREAD(conn);
    /* END TODO */
}

void flushInput() {
    char c;

    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

void *writerThread(void *conn) {
    int quit = 0;

    initscr();
    printf("Command (WASD, f=scan q=exit)\n");
    while (!quit) {
        char ch;
        ch = getch();

        char buffer[2];

        buffer[0] = NET_COMMAND_PACKET;
        switch (ch) {
            case 'w':
            case 'W':
            case 'a':
            case 'A':
            case 's':
            case 'S':
            case 'd':
            case 'D':
                buffer[1] = ch;
                sendData(conn, buffer, sizeof(buffer));
                break;
            case 'f':
            case 'F':
                buffer[1] = ch;
                sendData(conn, buffer, sizeof(buffer));
                break;
            case 'q':
            case 'Q':
                quit = 1;
                break;
            default:
                printf("BAD COMMAND\n");
        }
        flushinp();
        usleep(200000);
    }

    printf("Exiting keyboard thread\n");

    /* TODO: Stop the client loop and call EXIT_THREAD */
    endwin();
    stopClient();
    EXIT_THREAD(conn);
    /* END TODO */
}

/* TODO: #define filenames for the client private key, certificatea,
   CA filename, etc. that you need to create a client */

#define CA_CERT_FNAME "cert/signing.pem"
#define CLIENT_CERT_FNAME "controlkey/laptop.crt"
#define CLIENT_KEY_FNAME "controlkey/laptop.key"
#define SERVER_NAME_ON_CERT "www.alex.com"
/* END TODO */
void connectToServer(const char *serverName, int portNum) {
    /* TODO: Create a new client */
    createClient(serverName, portNum, 1, CA_CERT_FNAME, SERVER_NAME_ON_CERT, 1, CLIENT_CERT_FNAME,
                 CLIENT_KEY_FNAME, readerThread, writerThread);
    /* END TODO */
}

int main(int ac, char **av) {
    if (ac != 3) {
        fprintf(stderr, "\n\n%s <IP address> <Port Number>\n\n", av[0]);
        exit(-1);
    }

    networkActive = 1;
    connectToServer(av[1], atoi(av[2]));

    /* TODO: Add in while loop to prevent main from exiting while the
    client loop is running */

    while (client_is_running())
        ;
    /* END TODO */
    printf("\nMAIN exiting\n\n");
}
