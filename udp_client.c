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
#define SRV_PORT 10001


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
  if (sock < 0)
    syserr("socket");

  uint64_t tab[2], start;
  start = gettime();
  tab[0] = htobe64(start);
  int len = sizeof(tab);
  struct sockaddr_in my_address;
  socklen_t rcva_len = (socklen_t) sizeof(my_address);
  ssize_t snd_len;

  while(1) {
    if (pthread_rwlock_rdlock(&s->lock) != 0) {
        perror("reader_thread: pthread_rwlock_rdlock error");
        exit(__LINE__);
    }

    Node *ptr = s->head;
    printf("jestem: ");
    stack_print(&s->head);
    char buf[14];
    strcpy(buf, "hello");
    len = sizeof(buf);
    while(ptr) {
      snd_len = sendto(sock, buf, len, 0, (struct sockaddr *) &ptr->data, rcva_len);
      if (snd_len != len)
        syserr("partial / failed write");
      printf("wysłano do: ");
      print_ip_port(ptr->data);
      ptr = ptr->next;
    }
    if (pthread_rwlock_unlock(&s->lock) != 0) {
        perror("reader thread: pthred_rwlock_unlock error");
        exit(__LINE__);
    }
    printf("śpię...\n");
    sleep(1);
  }

    if (close(sock) == -1) { //very rare errors can occur here, but then
      syserr("close"); //it's healthy to do the check
    };

}




// int create_socket() {
//   struct addrinfo addr_hints;
//   struct addrinfo *addr_result;
//   uint64_t start, end;

//   int flags;
//   struct sockaddr_in my_address, srvr_address;
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
