#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "err.h"
#include "gettime.c"

int main(int argc, char *argv[])
{
  int sock, err;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;
  uint64_t start, end;

  if (argc < 3) {
    fatal("Usage: %s [-t|-u] host port ...\n", argv[0]);
  }

  if (strcmp("-t", argv[1]) == 0) { // TCP CONNECTION
    printf("*** TCP connection ***\n");

    // 'converting' host/port in string to struct addrinfo
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; // IPv4
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
    err = getaddrinfo(argv[2], argv[3], &addr_hints, &addr_result);
    if (err != 0)
      syserr("getaddrinfo: %s\n", gai_strerror(err));

    // initialize socket according to getaddrinfo results
    sock = socket(addr_result->ai_family,
      addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock < 0)
      syserr("socket");

    start = gettime();
    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
      syserr("connect");
    end = gettime();

    printf("Delay: %" PRIu64 " us\n", end - start);

    freeaddrinfo(addr_result);
    (void) close(sock);

  } else
  if (strcmp("-u", argv[1]) == 0) { // UDP CONNECTION
    printf("*** UDP connection ***\n");

    int flags;
    struct sockaddr_in my_address, srvr_address;
    ssize_t snd_len, rcv_len, len;
    socklen_t rcva_len;
    uint64_t tab[2];

    // 'converting' host/port in string to struct addrinfo
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; // IPv4
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;
    if (getaddrinfo(argv[2], NULL, &addr_hints, &addr_result) != 0) {
      syserr("getaddrinfo");
    }

    my_address.sin_family = AF_INET; // IPv4
    my_address.sin_addr.s_addr = ((struct sockaddr_in*)
      (addr_result->ai_addr))->sin_addr.s_addr; // address IP
    my_address.sin_port = htons((uint16_t) atoi(argv[3])); // Port

    freeaddrinfo(addr_result);

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
      syserr("socket");

    len = (ssize_t) sizeof(tab);
    flags = 0;
    rcva_len = (socklen_t) sizeof(my_address);

    start = gettime();
    tab[0] = htobe64(start);
    snd_len = sendto(sock, tab, len, flags, (struct sockaddr *) &my_address, rcva_len);
    if (snd_len != len) {
      syserr("partial / failed write");
    }
    rcv_len = recvfrom(sock, tab, len, flags,
        (struct sockaddr *) &srvr_address, &rcva_len);
    if (rcv_len < 0) {
      syserr("read");
    }
    end = gettime();

    printf("Delay: %" PRIu64 " us\n", end - start);

    fprintf(stderr, "send time: %" PRIu64 "\n", be64toh(tab[0]));
    fprintf(stderr, "rcvd time: %" PRIu64 "\n", be64toh(tab[1]));

    if (close(sock) == -1) { //very rare errors can occur here, but then
      syserr("close"); //it's healthy to do the check
    };

  } else
    fatal("Usage: %s [-t|-u] host port ...\n", argv[0]);

  return 0;
}
