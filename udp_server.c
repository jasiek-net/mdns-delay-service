#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <inttypes.h>

#include "threads.h"
#include "err.h"

#define __STDC_FORMAT_MACROS

void *udp_server(void *arg) {
  int sock, flags;
  struct sockaddr_in server_address, client_address;
  socklen_t snda_len, rcva_len;
  ssize_t len, snd_len;
  uint64_t tab[2];
  
  sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
  if (sock < 0) syserr("socket");
  int one = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  server_address.sin_family = AF_INET; // IPv4
  server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
  server_address.sin_port = htons(udp_port); // port for receiving from command line

  // bind the socket to a concrete address
  if (bind(sock, (struct sockaddr *) &server_address,
      (socklen_t) sizeof(server_address)) < 0)
    syserr("bind");
  
  snda_len = (socklen_t) sizeof(client_address);
  rcva_len = (socklen_t) sizeof(client_address);
  flags = 0; // we do not request anything special

  while(1) {
    len = recvfrom(sock, tab, sizeof(tab), flags, (struct sockaddr *) &client_address, &rcva_len);
    if (len < 0) syserr("error on datagram from client socket");
    else {
//      printf("rcvd time: %" PRIu64 "\n", be64toh(tab[0]));
      len = sizeof(tab);
      tab[1] = htobe64(gettime());
      snd_len = sendto(sock, tab, (size_t) len, flags, (struct sockaddr *) &client_address, snda_len);
      if (snd_len != len)
        syserr("error on sending datagram to client socket");
//      printf("send time: %" PRIu64 "\n", be64toh(tab[1]));
    }
  }  
  return 0;
}