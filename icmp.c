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

// this sets ICMP declaration
#include <netinet/ip_icmp.h>


#include "threads.h"

#define BSIZE 13
#define ICMP_HEADER_LEN 8

#define NOBODY_UID_GID 99




void send_ping_request(int sock, char* s_send_addr) {

  struct addrinfo addr_hints;
  struct addrinfo *addr_result;
  struct sockaddr_in send_addr;
  
  struct icmp* icmp;
  
  char send_buffer[BSIZE];
  
  int err = 0;
  ssize_t data_len = 0;
  ssize_t icmp_len = 0;
  ssize_t len = 0;
  
  // 'converting' host/port in string to struct addrinfo
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_RAW;
  addr_hints.ai_protocol = IPPROTO_ICMP;
  err = getaddrinfo(s_send_addr, 0, &addr_hints, &addr_result);
  if (err != 0)
    syserr("getaddrinfo: %s\n", gai_strerror(err));

  send_addr.sin_family = AF_INET;
  send_addr.sin_addr.s_addr =
      ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr;
  send_addr.sin_port = htons(0);
  freeaddrinfo(addr_result);

  memset(send_buffer, 0, sizeof(send_buffer));
  // initializing ICMP header
  icmp = (struct icmp *) send_buffer;
  icmp->icmp_type = ICMP_ECHO;
  icmp->icmp_code = 0;
  icmp->icmp_id = htons(getpid()); // process identified by PID
  icmp->icmp_seq = htons(0); // sequential number
  data_len = snprintf(((char*) send_buffer+ICMP_HEADER_LEN), sizeof(send_buffer)-ICMP_HEADER_LEN, "BASIC PING!");
  if (data_len < 1)
    syserr("snprinf");
  icmp_len = data_len + ICMP_HEADER_LEN; // packet is filled with 0
  icmp->icmp_cksum = 0; // checksum computed over whole ICMP package
  icmp->icmp_cksum = in_cksum((unsigned short*) icmp, icmp_len);

  len = sendto(sock, (void*) icmp, icmp_len, 0, (struct sockaddr *) &send_addr, 
               (socklen_t) sizeof(send_addr));
  if (icmp_len != (ssize_t) len)
    syserr("partial / failed write");

  printf("wrote %zd bytes\n", len);
}


int receive_ping_reply(int sock) {
  struct sockaddr_in rcv_addr;
  socklen_t rcv_addr_len;
  
  struct ip* ip;
  struct icmp* icmp;
  
  char rcv_buffer[BSIZE];
  
  ssize_t ip_header_len = 0;
  ssize_t data_len = 0;
  ssize_t icmp_len = 0;
  ssize_t len;
  

  memset(rcv_buffer, 0, sizeof(rcv_buffer));
  rcv_addr_len = (socklen_t) sizeof(rcv_addr);
  len = recvfrom(sock, (void*) rcv_buffer, sizeof(rcv_buffer), 0, 
                 (struct sockaddr *) &rcv_addr, &rcv_addr_len);
  
  printf("received %zd bytes from %s\n", len, inet_ntoa(rcv_addr.sin_addr));
  
  // recvfrom returns whole packet (with IP header)
  ip = (struct ip*) rcv_buffer;
  ip_header_len = ip->ip_hl << 2; // IP header len is in 4-byte words
  
  icmp = (struct icmp*) (rcv_buffer + ip_header_len); // ICMP header follows IP
  icmp_len = len - ip_header_len;

  if (icmp_len < ICMP_HEADER_LEN)
    fatal("icmp header len (%d) < ICMP_HEADER_LEN", icmp_len);
  
  if (icmp->icmp_type != ICMP_ECHOREPLY) {
    printf("strange reply type (%d)\n", icmp->icmp_type);
    return 0;
  }

  if (ntohs(icmp->icmp_id) != getpid())
    fatal("reply with id %d different from my pid %d", ntohs(icmp->icmp_id), getpid());

  data_len = len - ip_header_len - ICMP_HEADER_LEN;
  printf("correct ICMP echo reply; payload size %zd content %.*s\n", data_len,
         (int) data_len, (rcv_buffer+ip_header_len+ICMP_HEADER_LEN));
  return 1;
}

int main_W(int argc, char *argv[]) {
  int sock;
  
  if (argc < 2) {
    fatal("Usage: %s host\n", argv[0]);
  }

  sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock < 0)
    syserr("socket");

  drop_to_nobody();

  send_ping_request(sock, argv[1]);

  while (!receive_ping_reply(sock))
    ;

  if (close(sock) == -1) 
    syserr("close"); 

  return 0;
}



/*
Należy zaimplementować pomiar na podstawie czasu odpowiedzi na pakiety ICMP Echo Request.
W pakietach ICMP należy:
=> wpisać w polu identyfikator wartość 0x13.
=> W części data należy w pierwszych 24 bitach umieścić swój numer indeksu (zakodowany w BCD, w kolejności bajtów bigendian),
 a w ostatnich 8 - numer grupy. Należy zadbać o sensowną obsługę numerów sekwencyjnych. Program `opoznienia` nie powinien
odpowiadać na zapytania ICMP Echo Request. Zakładamy, że to potrafi system operacyjny.
*/
int sock;


void *icmp_receive(void *arg) {
  struct sockaddr_in rcv_addr;
  socklen_t rcv_addr_len;
  
  struct ip* ip;
  struct icmp* icmp;
  
  char rcv_buffer[BSIZE];
  
  ssize_t ip_header_len = 0;
  ssize_t data_len = 0;
  ssize_t icmp_len = 0;
  ssize_t len;
  

  memset(rcv_buffer, 0, sizeof(rcv_buffer));
  rcv_addr_len = (socklen_t) sizeof(rcv_addr);
  len = recvfrom(sock, (void*) rcv_buffer, sizeof(rcv_buffer), 0, (struct sockaddr *) &rcv_addr, &rcv_addr_len);
  
  printf("received %zd bytes from %s\n", len, inet_ntoa(rcv_addr.sin_addr));
  
  // recvfrom returns whole packet (with IP header)
  ip = (struct ip*) rcv_buffer;
  ip_header_len = ip->ip_hl << 2; // IP header len is in 4-byte words
  
  icmp = (struct icmp*) (rcv_buffer + ip_header_len); // ICMP header follows IP
  icmp_len = len - ip_header_len;

  if (icmp_len < ICMP_HEADER_LEN)
    fatal("icmp header len (%d) < ICMP_HEADER_LEN", icmp_len);
  
  if (icmp->icmp_type != ICMP_ECHOREPLY) {
    printf("strange reply type (%d)\n", icmp->icmp_type);
    return 0;
  }

  if (ntohs(icmp->icmp_id) != getpid())
    fatal("reply with id %d different from my pid %d", ntohs(icmp->icmp_id), getpid());

  data_len = len - ip_header_len - ICMP_HEADER_LEN;
  printf("correct ICMP echo reply; payload size %zd content %.*s\n", data_len,
         (int) data_len, (rcv_buffer+ip_header_len+ICMP_HEADER_LEN));
  return 1;
}

void *icmp (void *arg) {

  sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock < 0) syserr("socket");

  drop_to_nobody();

  struct addrinfo addr_hints;
  struct addrinfo *addr_result;
  struct sockaddr_in send_addr;
  
  struct icmp* icmp;
  
  char send_buffer[BSIZE];
  
  int err = 0;
  ssize_t data_len = 0;
  ssize_t icmp_len = 0;
  ssize_t len = 0;

// MESSAGE IN ICMP ECHO REQUEST:
// decimal:   3    1    0    7    5    4         4
// BCD:    0011 0001 0000 0111 0101 0100 0000 0100
// char:           1         G         T       EOT

  memset(send_buffer, 0, sizeof(send_buffer));
  icmp = (struct icmp *) send_buffer;
  icmp->icmp_type = ICMP_ECHO;
  icmp->icmp_code = 0;
  icmp->icmp_id = htons(0x13); // process identified by PID
  data_len = snprintf(((char*) send_buffer + ICMP_HEADER_LEN), sizeof(send_buffer) - ICMP_HEADER_LEN, "1\007T\004");
  printf("data_len %d\n", (int) data_len);
  if (data_len < 1) syserr("snprinf");

  icmp_len = data_len + ICMP_HEADER_LEN; // packet is filled with 0
  icmp->icmp_cksum = 0; // checksum computed over whole ICMP package
  icmp->icmp_cksum = in_cksum((unsigned short*) icmp, icmp_len);

  // memset(&addr_hints, 0, sizeof(struct addrinfo));
  // addr_hints.ai_family = AF_INET;
  // addr_hints.ai_socktype = SOCK_RAW;
  // addr_hints.ai_protocol = IPPROTO_ICMP;
  // err = getaddrinfo(s_send_addr, 0, &addr_hints, &addr_result);
  // if (err != 0) syserr("getaddrinfo: %s\n", gai_strerror(err));

  // send_addr.sin_family = AF_INET;
  // send_addr.sin_addr.s_addr = ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr;
  // send_addr.sin_port = htons(0);
  // freeaddrinfo(addr_result);


  // char a = '';
  // int i;
  // for (i = 0; i < 8; i++) {
  //     printf("%d", !!((a << i) & 0x80));
  // }
  // printf("\n");


  pthread_t icmp_receive_t;
  if (pthread_create(&icmp_receive_t, 0, icmp_receive, &sock) != 0) syserr("pthread_create");

  uint64_t start;

  while(1) {
    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");

    start = gettime();
    Node *p = head;
    while(p) {
      p->host.icm_seq = (p->host.icm_seq + 1) % 65536;
      p->host.icm_time = start;
      icmp->icmp_seq = htons( p->host.icm_seq ); // sequential number
      len = sendto(sock, (void*) icmp, icmp_len, 0, &p->host.addr, addr_len);
      if (icmp_len != (ssize_t) len) syserr("partial / failed write");
      printf("wrote %zd bytes\n", len);
      p = p->next;
    }

    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");

    sleep(delay);
  }

  if (close(sock) == -1) { //very rare errors can occur here, but then
    syserr("close"); //it's healthy to do the check
  };

}

void *udp_client_dfreceive(void *arg) {
  int sock = *(int *) arg;
  
  int flags = 0;
  struct sockaddr addr;
  socklen_t rcva_len;
  ssize_t len;
  uint64_t tab[2], end;
  
  rcva_len = (socklen_t) sizeof(addr);

  while(1) {
    len = recvfrom(sock, tab, sizeof(tab), flags, &addr, &rcva_len);    
    if (len < 0) syserr("error on datagram from client socket");
    else {
      end = gettime();
      printf("odebrano od: ");
      print_ip_port(addr);

		add_measurement(&addr, "udp", (int) (end - be64toh(tab[0])));

		printf("rcvd time: %" PRIu64 "\n", be64toh(tab[0]));
		printf("send time: %" PRIu64 "\n", be64toh(tab[1]));

    }
  }  
  return 0;
}
