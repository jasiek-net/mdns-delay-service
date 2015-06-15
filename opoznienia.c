#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <endian.h>
#include <signal.h>

#include "linkedlist.h"
#include "opoznienia.h"
#include "err.h"

pthread_t thread[6];

static void catch_int (int sig) {
  stack_clear();
  int i;
  for (i = 0; i < 6; i++)
    pthread_cancel(thread[i]);
  if (pthread_rwlock_destroy(&lock)) syserr("pthread_rwlock_destroy");
  if (pthread_rwlock_destroy(&lock_telnet)) syserr("pthread_rwlock_destroy");
}

int main(int argc, char *argv[]) {
  if (signal(SIGINT, catch_int) == SIG_ERR) syserr("signal");

  tcp_port = 22;
  udp_port = 3382; // -u
  telnet_port = 3637; // -U
  measure_delay = 1; // -t
  mdns_delay = 10; // -T
  telnet_delay = 1; // -v
  ssh_multicast = 0; // -s

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
  if (pthread_rwlock_init(&lock, NULL) != 0) syserr("pthread_rwlock_init");
  if (pthread_rwlock_init(&lock_telnet, NULL) != 0) syserr("pthread_rwlock_init");
  head = NULL;
  addr_len = sizeof(struct sockaddr);
  
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock < 0) syserr("socket");
  drop_to_nobody();

  // SHOW TIME!
  if (pthread_create(&thread[0], 0, telnet, NULL) != 0) syserr("pthread_create");
  if (pthread_create(&thread[1], 0, udp_server, NULL) != 0) syserr("pthread_create");
  if (pthread_create(&thread[2], 0, udp_client, NULL) != 0) syserr("pthread_create");
  if (pthread_create(&thread[3], 0, tcp_client, NULL) != 0) syserr("pthread_create");
  if (pthread_create(&thread[4], 0, icm_client, &sock) != 0) syserr("pthread_create");
  if (pthread_create(&thread[5], 0, mdns, NULL) != 0) syserr("pthread_create");

  printf("measure delay   -t %d\n", measure_delay);
  printf("mdns delay      -T %d\n", mdns_delay);
  printf("udp server port -u %d\n", udp_port);
  printf("telnet port     -U %d\n", telnet_port);
  printf("ssh multicast   -s %d\n", ssh_multicast);

  for (i = 0; i < 6; i++)
    pthread_join(thread[i], NULL);

  return 0;
}
