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
  struct stuff *s = (struct stuff *)arg;
  int port = 10001;

  int sock;
  struct sockaddr_in srvr_addr;
  struct sockaddr_in clin_addr;

  socklen_t rcva_len;

  sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
  if (sock < 0)
    syserr("socket");
  // after socket() call; we should close(sock) on any execution path;
  // since all execution paths exit immediately, sock would be closed when program terminates

  srvr_addr.sin_family = AF_INET; // IPv4
  srvr_addr.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
  srvr_addr.sin_port = htons(port); // default port for receiving is PORT_NUM

  // bind the socket to a concrete address
  if (bind(sock, (struct sockaddr *) &srvr_addr, (socklen_t) sizeof(srvr_addr)) < 0)
    syserr("bind");

  printf("Listen on: ");
  print_ip_port(srvr_addr);
  stack_print(&s->head);

  for (;;) {
    rcva_len = (socklen_t) sizeof(clin_addr);
    recvfrom(sock, (char *)NULL, (size_t)NULL, 0, (struct sockaddr *) &clin_addr, &rcva_len);
    if (pthread_rwlock_wrlock(&s->lock) != 0) {
          perror("writer thread: pthread_rwlock_wrlock error");
          exit(__LINE__);
      }

      printf("Add new host: ");
      stack_push(&s->head, clin_addr);
      stack_print(&s->head);

      if (pthread_rwlock_unlock(&s->lock) != 0) {
          perror("writer thread: pthread_rwlock_unlock error");
          exit(__LINE__);
      }
  }
  stack_clear(&s->head);
  return 0;
}