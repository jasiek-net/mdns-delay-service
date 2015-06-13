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

          printf("RCV Q name: %s, type: %d\n", q[i].name, ntohs(q[i].ques->qtype));
          create_answer(q[i].name, ntohs(q[i].ques->qtype), snd_buf, &len);
 
//          printf("wysłałem: %s\n", snd_buf);
 
          snd_len = sendto(sock, snd_buf, len, flags, &mdns_addr, addr_len);
          if (snd_len != len) syserr("error on sending datagram to client socket");        

        }
        free(q);
      } else if (ntohs(dns->ans_count)) { // we've got answer over here!
        a = get_answer(rcv_buf);
        for(i = 0; i < ntohs(dns->ans_count); i++) {
          if (ntohs(a[i].resource->type) == T_PTR) {

            printf("RCV PTR name: %s, rdata: %s, type: %d, size: %d\n", a[i].name, a[i].rdata, ntohs(a[i].resource->type), ntohs(a[i].resource->data_len));
            create_question(a[i].name, a[i].rdata, ntohs(a[i].resource->type), snd_buf, &len);

          } else if (ntohs(a[i].resource->type) == T_A) {
            long *p;
            p=(long*)a[i].rdata;
            struct sockaddr_in a;
            a.sin_addr.s_addr=(*p);
            printf("RCV A IPv4 address : %s\n",inet_ntoa(a.sin_addr));
            len = 0;
          }

          if (len != 0) {
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

        // struct sockaddr_in a;
        // printf("\nAnswer Records : %d \n" , ntohs(dns->ans_count) );
        // for(i=0 ; i < ntohs(dns->ans_count) ; i++) {
        //     printf("Name : %s \n", answers[i].name);
        //     printf("  type : %d \n", ntohs(answers[i].resource->type));
        //     printf("  _class : %d \n", ntohs(answers[i].resource->_class));
        //     printf("  _ttl : %d \n", ntohs(answers[i].resource->ttl));
        //     printf("  data_len : %d \n", ntohs(answers[i].resource->data_len));

        //     if( ntohs(answers[i].resource->type) == T_A) { //IPv4 address
        //         long *p;
        //         p = (long*)answers[i].rdata;
        //         a.sin_addr.s_addr=(*p); //working without ntohl
        //         printf("  has IPv4 address : %s\n",inet_ntoa(a.sin_addr));
        //     }
             

}

void *mdns(void *arg) {
  int sock, flags = 0;

  unsigned char buf[MAX_BUF_SIZE];
  unsigned char search_opo[] = "_opoznienia._udp.local.";
  
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

  ssize_t snd_len, len;
  get_record(search_opo, T_PTR, 0, buf, &len); // 0 means query
//  printf("size: %d\n", len);

 // parse_msg(msg);
  while(1) {
    snd_len = sendto(sock, buf, len, flags, &mdns_addr, addr_len);
    if (snd_len != len) syserr("sendto");
//    printf("1 Q T_PTR: %s\n", search_opo);
    sleep(mdns_delay);    
  }

  if (close(sock) == -1) syserr("close");

  return NULL;
}