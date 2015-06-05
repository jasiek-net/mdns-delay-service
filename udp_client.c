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

struct udp_struct {
  struct stuff *s;
  int *b;
};

void *udp_client_receive(void *arg);

void *udp_client (void *arg) {
  struct stuff *s = (struct stuff *)arg;

  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) syserr("socket");

  uint64_t tab[2];
  int len = sizeof(tab);
  struct sockaddr_in addr;
  socklen_t rcva_len = (socklen_t) sizeof(addr);
  ssize_t snd_len;

  addr.sin_family = AF_INET; // IPv4
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // address IP
  addr.sin_port = 0; // Port

  if (bind(sock, (struct sockaddr *) &addr, rcva_len) < 0) syserr("bind");

  struct udp_struct rcv;
  rcv.s = s;
  rcv.b = malloc(sizeof(int));
  *rcv.b = sock;
  pthread_t udp_client_receive_t;
  if (pthread_create(&udp_client_receive_t, 0, udp_client_receive, &rcv) != 0) syserr("pthread_create");


  while(1) {
    if (pthread_rwlock_rdlock(&s->lock) != 0) syserr("pthread_rwlock_rdlock error");

    Node *ptr = s->head;
    stack_print(&s->head);
    tab[0] = htobe64( gettime() );
    while(ptr) {
      snd_len = sendto(sock, tab, len, 0, (struct sockaddr *) &ptr->host.addr, rcva_len);
      if (snd_len != len) syserr("partial / failed write");
      printf("wysłano do: ");
      print_ip_port(ptr->host.addr);
      ptr = ptr->next;
    }

    if (pthread_rwlock_unlock(&s->lock) != 0) syserr("pthred_rwlock_unlock error");

    printf("śpię...\n");
    sleep(1);
  }

  if (close(sock) == -1) { //very rare errors can occur here, but then
    syserr("close"); //it's healthy to do the check
  };

}

void *udp_client_receive(void *arg) {
  struct udp_struct *rcv = (struct udp_struct *) arg;
  struct stuff *s = rcv->s;
  int sock = *(int *) rcv->b;
  free(rcv->b);
  
  int flags = 0;
  struct sockaddr_in addr;
  socklen_t rcva_len;
  ssize_t len;
  uint64_t tab[2], end;
  
  rcva_len = (socklen_t) sizeof(addr);

  while(1) {
    len = recvfrom(sock, tab, sizeof(tab), flags, (struct sockaddr *) &addr, &rcva_len);    
    if (len < 0) syserr("error on datagram from client socket");
    else {
      end = gettime();
      printf("odebrano od: ");
      print_ip_port(addr);

      if (pthread_rwlock_wrlock(&s->lock) != 0) syserr("pthread_rwlock_wrlock error");

        printf("rcvd time: %" PRIu64 "\n", be64toh(tab[0]));
        printf("send time: %" PRIu64 "\n", be64toh(tab[1]));

      if (pthread_rwlock_unlock(&s->lock) != 0) syserr("pthred_rwlock_unlock error");

    }
  }  
  return 0;
}