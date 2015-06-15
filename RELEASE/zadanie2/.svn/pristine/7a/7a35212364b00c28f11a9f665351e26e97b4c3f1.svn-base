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

#include "linkedlist.h"
#include "opoznienia.h"
#include "err.h"

void print_ip_port(struct sockaddr sa) {
	(void) printf("%s", inet_ntoa(((struct sockaddr_in *) &sa)->sin_addr));
	(void) printf(":");
	(void) printf("%d\n", ntohs(((struct sockaddr_in *) &sa)->sin_port));
}

uint64_t gettime() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*1000000ull+tv.tv_usec;
}

struct udp_struct {
  struct stuff *s;
  int *b;
};

void *udp_client_receive(void *arg);

void *udp_client (void *arg) {

  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) syserr("socket");

  uint64_t tab[2];
  int len = sizeof(tab);
  struct sockaddr addr;
  socklen_t rcva_len = (socklen_t) sizeof(addr);
  ssize_t snd_len;

  ((struct sockaddr_in *) &addr)->sin_family = AF_INET; // IPv4
  ((struct sockaddr_in *) &addr)->sin_addr.s_addr = htonl(INADDR_ANY); // address IP
  ((struct sockaddr_in *) &addr)->sin_port = htons(0); // Port

  if (bind(sock, &addr, rcva_len) < 0) syserr("bind");

  pthread_t udp_client_receive_t;
  if (pthread_create(&udp_client_receive_t, 0, udp_client_receive, &sock) != 0) syserr("pthread_create");

  while(1) {
    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");

    Node *p = head;
    tab[0] = htobe64( gettime() );
    while(p) {
      if (p->host.is_udp) {
        snd_len = sendto(sock, tab, len, 0, &p->host.addr_udp, rcva_len);
        if (snd_len != len) syserr("partial / failed write");
        // printf("udp sendto: ");
        // print_ip_port(p->host.addr_udp);        
      }
      p = p->next;
    }

    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");

    sleep(measure_delay);
  }

  if (close(sock) == -1) syserr("close");

}

void *udp_client_receive(void *arg) {
  int sock = *(int *) arg;
  
  int flags = 0;
  struct sockaddr addr;
  socklen_t rcva_len;
  ssize_t len;
  uint64_t tab[2];
  
  rcva_len = (socklen_t) sizeof(addr);

  while(1) {
    len = recvfrom(sock, tab, sizeof(tab), flags, &addr, &rcva_len);    
    if (len < 0) syserr("error on datagram from client socket");
    else {
    	add_udp_measurement(&addr, gettime() - be64toh(tab[0]));
      // printf("udp recvfr: ");
      // print_ip_port(addr);
  		// printf("rcvd time: %" PRIu64 "\n", be64toh(tab[0]));
  		// printf("send time: %" PRIu64 "\n", be64toh(tab[1]));
    }
  }  
  return 0;
}
