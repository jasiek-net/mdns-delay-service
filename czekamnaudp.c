#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>
#include <endian.h>

#include "lib/err.h"
#include "lib/get_time_usec.h"      // get time since epoch in microseconds

#define PORT_MIN    1024
#define PORT_MAX    65535

/* Parse arguments and return port number. */
unsigned short parse_arguments(int argc, char *argv[]) {
    int port_num;
    if (argc != 2)
        fatal("Usage: %s port\n", argv[0]);

    port_num = atoi(argv[1]);
    /* handle the case of invalid conversion (port_num = 0)
       or port number out of range: */
    if (port_num < PORT_MIN || port_num > PORT_MAX) {
        fatal("Port number must be an integer between %d and %d\n",
                PORT_MIN, PORT_MAX);        
    }
    return port_num;
}

int main(int argc, char *argv[]) {
    uint64_t time_buffer[2];        // buffer to receive 1 and send 2 timestamps
    int sock;
    unsigned short port_num;

    size_t len1, len2;
    ssize_t snd_len, rcv_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t snda_len, rcva_len;

    port_num = parse_arguments(argc, argv);

    sock = socket(AF_INET, SOCK_DGRAM, 0);              // UDP socket
    if (sock < 0)
        syserr("socket");

    server_address.sin_family = AF_INET;                // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // all interfaces
    server_address.sin_port = htons(port_num);          // port for receiving

    if (bind(sock, (struct sockaddr *) &server_address,
            (socklen_t) sizeof(server_address)) < 0)
        syserr("bind");

    snda_len = (socklen_t) sizeof(client_address);
    rcva_len = snda_len;
    len1 = (size_t) sizeof(uint64_t);   // one timestamp
    len2 = 2 * len1;                    // two timestamps

    /* receive one and send two timestamps: */
    for (;;) {
        rcv_len = recvfrom(sock, time_buffer, len1, 0,
                (struct sockaddr *) &client_address, &rcva_len);
        if (rcv_len < 0)
            syserr("error on datagram from client socket");
        
        time_buffer[1] = htobe64(get_time_usec());   // add big-endian timestamp

        snd_len = sendto(sock, time_buffer, len2, 0,
                (struct sockaddr *) &client_address, snda_len);
        if (snd_len != len2)
            syserr("error on sending datagram to client socket");
        
    }

    return 1;       // shouldn't get here
}
