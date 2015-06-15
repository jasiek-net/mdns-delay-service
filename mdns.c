#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
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
#include <net/if.h>

#include "threads.h"
#include "mdns_extra.h"

struct sockaddr mdns_addr;

void *m_dns_receive(void *arg) {
  int sock = *(int *) arg;
  struct sockaddr addr;
  int flags = 0, i;

  socklen_t snd_len;
  ssize_t len;
  unsigned char rcv_buf[MAX_BUF_SIZE];
  unsigned char snd_buf[MAX_BUF_SIZE];
  struct DNS_HEADER *dns = NULL;
  struct RES_RECORD *a;
  struct QUERY *q;

  while(1) {
    len = recvfrom(sock, &rcv_buf, sizeof(rcv_buf), flags, &addr, &addr_len);
    if (len < 0) syserr("error on datagram from client socket");
    else {
      dns = (struct DNS_HEADER*) rcv_buf;

      if (ntohs(dns->q_count)) {  // we've got question over here!
        q = get_question(rcv_buf);
        for (i = 0; i < ntohs(dns->q_count); i++) {
          // printf("RCV Q name: %s, type: %d\n", q[i].name, ntohs(q[i].ques->qtype));

          if (create_answer(q[i].name, ntohs(q[i].ques->qtype), snd_buf, &len)) {
            // printf("wchodze\n");
            snd_len = sendto(sock, snd_buf, len, flags, &mdns_addr, addr_len);
            if (snd_len != len) syserr("error on sending datagram to client socket");        
          }
        }
        free(q);
      } else if (ntohs(dns->ans_count)) { // we've got answer over here!
        a = get_answer(rcv_buf);
        for(i = 0; i < ntohs(dns->ans_count); i++) {
          // if (ntohs(a[i].resource->type) == T_PTR)
          //     printf("RCV A name: %s, rdata: %s, type: %d, size: %d\n", a[i].name, a[i].rdata, ntohs(a[i].resource->type), ntohs(a[i].resource->data_len));

          if (create_question(a[i].name, a[i].rdata, ntohs(a[i].resource->type), snd_buf, &len)) {
            snd_len = sendto(sock, snd_buf, len, flags, &mdns_addr, addr_len);
            if (snd_len != len) syserr("error on sending datagram to client socket");                                  
          }
        }
        free(a);
      }
    }
  }
  (void) printf("finished exchange\n");
  return 0;
}

void *mdns(void *arg) {
  set_my_ip();
  set_my_host();
  int sock, flags = 0;

  unsigned char buf_udp[MAX_BUF_SIZE];
  unsigned char buf_tcp[MAX_BUF_SIZE];
  
  sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
  if (sock < 0) syserr("socket");
  int one = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  ((struct sockaddr_in *) &mdns_addr)->sin_family = AF_INET;
  ((struct sockaddr_in *) &mdns_addr)->sin_addr.s_addr = inet_addr("224.0.0.251");
  ((struct sockaddr_in *) &mdns_addr)->sin_port = htons(5353);

  if (bind(sock, &mdns_addr, addr_len) < 0) syserr("bind");

  pthread_t m_dns_receive_t;
  if (pthread_create(&m_dns_receive_t, 0, m_dns_receive, &sock) != 0) syserr("pthread_create");

  ssize_t snd_len, len_udp, len_tcp;
  create_question("_opoznienia._udp.local.", "x", 0, buf_udp, &len_udp);
  create_answer("_ssh._tcp.local.", T_PTR, buf_tcp, &len_tcp);

  while(1) {
    stack_check();
    snd_len = sendto(sock, buf_udp, len_udp, flags, &mdns_addr, addr_len);
    if (snd_len != len_udp) syserr("sendto");
    if (ssh_multicast) {
      snd_len = sendto(sock, buf_tcp, len_tcp, flags, &mdns_addr, addr_len);
      if (snd_len != len_tcp) syserr("sendto");      
    }
    sleep(mdns_delay);
  }

  if (close(sock) == -1) syserr("close");

  return NULL;
}