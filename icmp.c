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

#define BSIZE 1000
#define ICMP_HEADER_LEN 8
#define NOBODY_UID_GID 99

void *icmp_receive(void *arg) {
  int sock = *(int *) arg;

  struct sockaddr rcv_addr;
  socklen_t rcv_addr_len;
  
  struct ip* ip;
  struct icmp* icmp;
  
  char rcv_buffer[BSIZE];
  
  ssize_t ip_header_len = 0;
  // ssize_t data_len = 0;
  ssize_t icmp_len = 0;
  ssize_t len;
  
  memset(rcv_buffer, 0, sizeof(rcv_buffer));
  rcv_addr_len = (socklen_t) sizeof(rcv_addr);

  while(1) {
    len = recvfrom(sock, (void*) rcv_buffer, sizeof(rcv_buffer), 0, &rcv_addr, &rcv_addr_len);
    
    // recvfrom returns whole packet (with IP header)
    ip = (struct ip*) rcv_buffer;
    ip_header_len = ip->ip_hl << 2; // IP header len is in 4-byte words
    
    icmp = (struct icmp*) (rcv_buffer + ip_header_len); // ICMP header follows IP
    icmp_len = len - ip_header_len;

    if (icmp_len >= ICMP_HEADER_LEN)
    if (icmp->icmp_type == ICMP_ECHOREPLY)
    if (ntohs(icmp->icmp_id) == 0x13) {
      // data_len = len - ip_header_len - ICMP_HEADER_LEN;
      // printf("icm recvfr: ");
      // print_ip_port(rcv_addr);
      add_icm_measurement(&rcv_addr, gettime()); 
    }
  }
}

void *icm_client(void *arg) {
  int sock = *(int *) arg;
  
  struct icmp* icmp;
  
  char send_buffer[BSIZE];
  
  ssize_t data_len = 0;
  ssize_t icmp_len = 0;
  ssize_t len = 0;

  memset(send_buffer, 0, sizeof(send_buffer));
  icmp = (struct icmp *) send_buffer;
  icmp->icmp_type = ICMP_ECHO;
  icmp->icmp_code = 0;
  icmp->icmp_seq = htons(0); // sequential number
  icmp->icmp_id = htons(0x13); // process identified by PID
  data_len = snprintf(((char*) send_buffer + ICMP_HEADER_LEN), sizeof(send_buffer) - ICMP_HEADER_LEN, "1\007T\004");
  printf("data_len %d\n", (int) data_len);
  if (data_len < 1) syserr("snprinf");

  icmp_len = data_len + ICMP_HEADER_LEN; // packet is filled with 0
  icmp->icmp_cksum = 0; // checksum computed over whole ICMP package
  icmp->icmp_cksum = in_cksum((unsigned short*) icmp, icmp_len);

  pthread_t icmp_receive_t;
  if (pthread_create(&icmp_receive_t, 0, icmp_receive, &sock) != 0) syserr("pthread_create");

  while(1) {
    if (pthread_rwlock_wrlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");

    Node *p = head;
    while(p) {
      if (p->host.is_icm) {
        p->host.icm_time = gettime();
        len = sendto(sock, (void*) icmp, icmp_len, 0, &p->host.addr_icm, addr_len);
        if (icmp_len != (ssize_t) len) syserr("partial / failed write");
        // printf("icm sendto: ");
        // print_ip_port(p->host.addr_icm);        
      }
      p = p->next;
    }

    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");

    sleep(measure_delay);
  }

  if (close(sock) == -1) { //very rare errors can occur here, but then
    syserr("close"); //it's healthy to do the check
  };

}