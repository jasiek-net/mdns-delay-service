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

#include <sys/time.h>
#include <inttypes.h>
#define __STDC_FORMAT_MACROS

uint64_t gettime() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*1000000ull+tv.tv_usec;
}

/* get address information */
struct addrinfo * get_addr_info(const char* host, const char* port, int socktype, int protocol) {
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


int main(int argc, char *argv[]) {
  int sock, flags;
  struct sockaddr_in server_address, client_address;
  socklen_t snda_len, rcva_len;
  ssize_t len, snd_len;
  uint64_t tab[2];
  
  sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
  if (sock < 0) syserr("socket");
  // after socket() call; we should close(sock) on any execution path;
  // since all execution paths exit immediately, sock would be closed when program terminates

  server_address.sin_family = AF_INET; // IPv4
  server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
  server_address.sin_port = 0; //htons(PORT); // port for receiving from command line any port!

  // bind the socket to a concrete address
  if (bind(sock, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0)
    syserr("bind");
  
  struct addrinfo *addr_result = get_addr_info("localhost", "10001", SOCK_DGRAM, IPPROTO_UDP);

  client_address.sin_family = AF_INET;
  client_address.sin_addr.s_addr = ((struct sockaddr_in *)(addr_result->ai_addr))->sin_addr.s_addr; // address IP
  client_address.sin_port = htons((uint16_t) atoi("10001"));

  snda_len = (socklen_t) sizeof(client_address);
  rcva_len = (socklen_t) sizeof(client_address);
  flags = 0; // we do not request anything special
  len = sizeof(tab);
  snd_len = sendto(sock, tab, (size_t) len, flags, (struct sockaddr *) &client_address, snda_len);
  if (snd_len != len)
      syserr("sendto");
  printf("wysłano\n");

  while(1) {
    snda_len = (socklen_t) sizeof(client_address);
    rcva_len = (socklen_t) sizeof(client_address);
    flags = 0; // we do not request anything special
    len = recvfrom(sock, tab, sizeof(tab), flags, (struct sockaddr *) &client_address, &rcva_len);
    if (len < 0)
      syserr("error on datagram from client socket");
    else {
      printf("rcvd time: %" PRIu64 "\n", be64toh(tab[0]));
      len = sizeof(tab);
      tab[1] = htobe64(gettime());
      snd_len = sendto(sock, tab, (size_t) len, flags,
          (struct sockaddr *) &client_address, snda_len);
      if (snd_len != len)
        syserr("error on sending datagram to client socket");
      printf("send time: %" PRIu64 "\n", be64toh(tab[1]));
    }  
  }

  return 0;
}