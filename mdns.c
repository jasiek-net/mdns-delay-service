#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <stdlib.h> // exit() function

#include "threads.h"

void *mdns(void * arg) {
  int port = 10001;

  int sock;
  struct sockaddr srvr_addr;
  struct sockaddr clin_addr;

  socklen_t rcva_len;

  sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
  if (sock < 0) syserr("socket");

  ((struct sockaddr_in *) &srvr_addr)->sin_family = AF_INET; // IPv4
  ((struct sockaddr_in *) &srvr_addr)->sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
  ((struct sockaddr_in *) &srvr_addr)->sin_port = htons(port); // default port for receiving is PORT_NUM

  if (bind(sock, &srvr_addr, (socklen_t) sizeof(srvr_addr)) < 0) syserr("bind");

  printf("Listen on: ");
  print_ip_port(srvr_addr);
  stack_print();

  for (;;) {
    rcva_len = (socklen_t) sizeof(clin_addr);
    recvfrom(sock, (char *)NULL, (size_t)NULL, 0, &clin_addr, &rcva_len);
 
      stack_push(clin_addr);
      printf("Add new host: ");
      stack_print();

  }

  stack_clear();
  return 0;
}
