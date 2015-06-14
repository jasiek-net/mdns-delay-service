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
  tcp_port = 22;
  udp_port = 3382;    // -u
  telnet_port = 3637;  // -U
  measure_delay = 1;  // -t
  mdns_delay = 10;     // -T
  telnet_delay = 1;   // -v
  ssh_multicast = 0;  // -s

  // reading arguments:
  int i;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-u") && i + 1 < argc)
      sscanf(argv[i+1], "%d", &udp_port);
    else if (!strcmp(argv[i], "-U") && i + 1 < argc)
      sscanf(argv[i+1], "%d", &telnet_port);
    else if (!strcmp(argv[i], "-t") && i + 1 < argc)
      sscanf(argv[i+1], "%d", &measure_delay);
    else if (!strcmp(argv[i], "-T") && i + 1 < argc)
      sscanf(argv[i+1], "%d", &mdns_delay);
    if (!strcmp(argv[i], "-v") && i + 1 < argc)
      sscanf(argv[i+1], "%d", &telnet_delay);      
    if (!strcmp(argv[i], "-s"))
      ssh_multicast = 1;
  }

  // initialization:
  if (pthread_rwlock_init(&lock, NULL) != 0)
    syserr("pthread_rwlock_init");
  if (pthread_rwlock_init(&lock_telnet, NULL) != 0)
    syserr("pthread_rwlock_init");
  if (pthread_rwlock_init(&lock_tcp, NULL) != 0)
    syserr("pthread_rwlock_init");
  head = NULL;
  addr_len = sizeof(struct sockaddr);

  pthread_t udp_server_t;
  pthread_t udp_client_t;
  pthread_t tcp_client_t;
  pthread_t icm_client_t;
  pthread_t mdns_t;
  pthread_t telnet_t;
  
  // SHOW TIME!
  // if (pthread_create(&udp_server_t, 0, udp_server, NULL) != 0)
  //   syserr("pthread_create");

  // if (pthread_create(&udp_client_t, 0, udp_client, NULL) != 0) syserr("pthread_create");

  // if (pthread_create(&tcp_client_t, 0, tcp_client, NULL) != 0)
  //   syserr("pthread_create");

  // if (pthread_create(&icm_client_t, 0, icm_client, NULL) != 0)
  //   syserr("pthread_create");

  mdns_delay = 2;
  if (pthread_create(&mdns_t, 0, mdns, NULL) != 0)
    syserr("pthread_create");
  
  // if (pthread_create(&telnet_t, 0, telnet, NULL) != 0)
  //   syserr("pthread_create");

 
  // pthread_t udp_client_t;
  // if (pthread_create(&udp_client_t, 0, udp_client, &s) != 0) {
  //   perror("pthread_create error");
  //   exit (__LINE__);
  // }
  // pthread_detach(udp_client_t);
  
  // pthread_join(telnet_t, NULL);

  while(1)
    sleep(1);

  return 0;
}
