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


int main(int argc, char *argv[]) {
  if (pthread_rwlock_init(&lock, NULL) != 0) syserr("pthread_rwlock_init");
  if (pthread_rwlock_init(&lock_telnet, NULL) != 0) syserr("pthread_rwlock_init");
  if (pthread_rwlock_init(&lock_tcp, NULL) != 0) syserr("pthread_rwlock_init");

  head = NULL;

  addr_len = sizeof(struct sockaddr);
  delay = 1;

  int *udp_srv_port;
  udp_srv_port = malloc(sizeof(int));
  *udp_srv_port = 3382;

  // pthread_t udp_server_t;
  // if (pthread_create(&udp_server_t, 0, udp_server, udp_srv_port) != 0) syserr("pthread_create");

//  pthread_t m_dns_t;
//  if (pthread_create(&m_dns_t, 0, m_dns, &sec) != 0) syserr("pthread_create");

 pthread_t mdns_t;
 if (pthread_create(&mdns_t, 0, mdns, head) != 0) syserr("pthread_create");

  // pthread_t udp_client_t;
  // if (pthread_create(&udp_client_t, 0, udp_client, head) != 0) syserr("pthread_create");

  // pthread_t tcp_client_t;
  // if (pthread_create(&tcp_client_t, 0, tcp_client, &sec) != 0) syserr("pthread_create");

 // pthread_t icmp_t;
 // if (pthread_create(&icmp_t, 0, icmp, NULL) != 0) syserr("pthread_create");

 telnet_delay = 1;
 telnet_port = 3637;
 pthread_t telnet_t;
 if (pthread_create(&telnet_t, 0, telnet, telnet_port) != 0) syserr("pthread_create");

  // pthread_t udp_client_t;
  // if (pthread_create(&udp_client_t, 0, udp_client, &s) != 0) {
  //   perror("pthread_create error");
  //   exit (__LINE__);
  // }
  // pthread_detach(udp_client_t);
  


  // create_thread(mdns_t, mdns);
  // create_thread(udp_client_t, udp_client);
  pthread_join(telnet_t, NULL);
  return 0;
}
