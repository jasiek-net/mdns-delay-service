#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>
#include <endian.h>

#include "err.h"
#include "get_time_usec.h"      // get time since epoch in microseconds

/* get address information */
struct addrinfo * get_addr_info(const char* host, const char* port,
                                int socktype, int protocol);
/* Measure and return the time of connecting with TCP protocol. */
uint64_t measure_tcp(int sock, struct addrinfo *addr_result);
/* Measure and return the time of sending and receiving UDP datagram. */
uint64_t measure_udp(int sock, struct addrinfo *addr_result);

/* Parse arguments and return protocol type: IPPROTO_UDP or IPPROTO_TCP. */
int parse_arguments(int argc, char *argv[]) {
    if (argc != 4)
        fatal("Usage: %s connection_type host port\n", argv[0]);

    if (strcmp(argv[1], "-u") == 0)
        return IPPROTO_UDP;
    else if (strcmp(argv[1], "-t") == 0)
        return IPPROTO_TCP;
    else
        fatal("Unknown connection type. Usage:\n"
              "%s -t host port        for TCP diagnostic\n"
              "%s -u host port        for UDP diagnostic\n", argv[0], argv[0]);

    return -1;
}

int main(int argc, char *argv[]) {
    int sock;
    int protocol, socktype;
    struct addrinfo *addr_result;
    uint64_t time_diff;

    /* Set up protocol type: */
    protocol = parse_arguments(argc, argv);
    if (protocol == IPPROTO_UDP)                // UDP
        socktype = SOCK_DGRAM;
    else if (protocol == IPPROTO_TCP)           // TCP
        socktype = SOCK_STREAM;

    addr_result = get_addr_info(argv[2], argv[3], socktype, protocol);

    sock = socket(addr_result->ai_family, addr_result->ai_socktype,
            addr_result->ai_protocol);    
    if (sock < 0)
        syserr("socket");

    if (protocol == IPPROTO_UDP)
        time_diff = measure_udp(sock, addr_result);
    else
        time_diff = measure_tcp(sock, addr_result);

    printf("%" PRIu64 "\n", time_diff);     // final result

    if (close(sock) == -1)
        syserr("close");
	return 0;
}

/* get address information */
struct addrinfo * get_addr_info(const char* host, const char* port,
                                int socktype, int protocol) {
    int err;
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;

    /* 'converting' host/port in string to struct addrinfo */
    (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; // IPv4
    addr_hints.ai_socktype = socktype;
    addr_hints.ai_protocol = protocol;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;
    err = (getaddrinfo(host, port, &addr_hints, &addr_result) != 0);
    if (err != 0)
        syserr("getaddrinfo: %s\n", gai_strerror(err));

    return addr_result;
}

/* Measure and return the time of connecting with TCP protocol. */
uint64_t measure_tcp(int sock, struct addrinfo *addr_result) {
    uint64_t time1, time2;         // measure time difference

    time1 = get_time_usec();
    
    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
        syserr("connect");

    time2 = get_time_usec();

    freeaddrinfo(addr_result);

    return time2 - time1;
}

/* Measure and return the time of sending and receiving UDP datagram. */
uint64_t measure_udp(int sock, struct addrinfo *addr_result) {
    uint64_t time1, time2;         // measure time difference
    uint64_t time_buffer[2];       // buffer to receive 2 timestamps

    size_t len1, len2;
    ssize_t snd_len, rcv_len;

    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
        syserr("connect");

    freeaddrinfo(addr_result);


    len1 = (size_t) sizeof(uint64_t);   // one timestamp
    len2 = 2 * len1;                    // two timestamps

    time1 = get_time_usec();
    time_buffer[0] = htobe64(time1);    // big-endian

    snd_len = write(sock, (void *) time_buffer, len1); // send one timestamp
    if (snd_len != len1)
        syserr("partial / failed write");

    rcv_len = read(sock, (void *) time_buffer, len2);  // receive two timestamps

    time2 = get_time_usec();

    if (rcv_len < 0)
        syserr("read");

    /* Diagnostic message, we receive big-endian numbers: */
    fprintf(stderr, "%" PRIu64 " %" PRIu64 "\n", be64toh(time_buffer[0]),
                                                 be64toh(time_buffer[1]));

    return time2 - time1;
}