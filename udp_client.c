#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <inttypes.h>
#include <endian.h>

#include "threads.h"

#define SRV_IP "localhost"
#define SRV_PORT 3382


uint64_t gettime() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*1000000ull+tv.tv_usec;
}

void print_ip_port(struct sockaddr_in sa) {
  (void) printf("%s", inet_ntoa(sa.sin_addr));
  (void) printf(":");
  (void) printf("%d\n", ntohs(sa.sin_port));
}

void *udp_client (void *arg) {
  struct stuff *s = (struct stuff *)arg;

  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) syserr("socket");

  uint64_t tab[2];

  while(1) {
    if (pthread_rwlock_rdlock(&s->lock) != 0) syserr("pthread_rwlock_rdlock error");

    Node *ptr = s->head;
    stack_print(&s->head);
    tab[0] = htobe64( gettime() );
    while(ptr) {
      int len = sendto(sock, tab, sizeof(tab), 0, (struct sockaddr *) &ptr->data, (socklen_t) sizeof(&ptr->data));
      if (len != sizeof(tab)) syserr("partial / failed write");
      printf("wysłano do: ");
      print_ip_port(ptr->data);
      ptr = ptr->next;
    }

    if (pthread_rwlock_unlock(&s->lock) != 0) syserr("pthred_rwlock_unlock error");

    printf("śpię...\n");
    sleep(1);
  }

  if (close(sock) == -1) syserr("close");

  return 0;
}

void *udp_server(void *arg) {
  int port = *(int *) arg;
  free(arg);
  
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
  server_address.sin_port = htons(port); // port for receiving from command line

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
    printf("rcvd time: %" PRIu64 "\n", be64toh(tab[0]));
    len = sizeof(tab);
    tab[1] = htobe64(gettime());
    snd_len = sendto(sock, tab, (size_t) len, flags, (struct sockaddr *) &client_address, snda_len);
    if (snd_len != len)
      syserr("error on sending datagram to client socket");
    printf("send time: %" PRIu64 "\n", be64toh(tab[1]));
  }
  }  
  return 0;
}




// int create_socket() {
//   struct addrinfo addr_hints;
//   struct addrinfo *addr_result;
//   uint64_t start, end;

//   int flags;
//   struct sockaddr_in my_address, address;
//   ssize_t snd_len, rcv_len, len;
//   socklen_t rcva_len;
//   uint64_t tab[2];

//   // 'converting' host/port in string to struct addrinfo
//   memset(&addr_hints, 0, sizeof(struct addrinfo));
//   addr_hints.ai_family = AF_INET; // IPv4
//   addr_hints.ai_socktype = SOCK_DGRAM;
//   addr_hints.ai_protocol = IPPROTO_UDP;
//   addr_hints.ai_flags = 0;
//   addr_hints.ai_addrlen = 0;
//   addr_hints.ai_addr = NULL;
//   addr_hints.ai_canonname = NULL;
//   addr_hints.ai_next = NULL;
//   if (getaddrinfo(SRV_IP, NULL, &addr_hints, &addr_result) != 0) {
//     syserr("getaddrinfo");
//   }

//   my_address.sin_family = AF_INET; // IPv4
//   my_address.sin_addr.s_addr = ((struct sockaddr_in*)
//     (addr_result->ai_addr))->sin_addr.s_addr; // address IP
//   my_address.sin_port = htons((uint16_t) SRV_PORT); // Port

//   freeaddrinfo(addr_result);

// }
