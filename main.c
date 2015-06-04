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

void create_thread(pthread_t t, void *(* func)(void *), struct stuff s) {
  if (pthread_create(&t, 0, func, &s) != 0) {
    perror("pthread_create error");
    exit (__LINE__);
  }
  pthread_detach(t);
}

int main(int argc, char *argv[]) {
  struct stuff s;
  s.head = NULL;
  if (pthread_rwlock_init(&s.lock, NULL) != 0) syserr("pthread_rwlock_init");

  pthread_t mdns_t;
  if (pthread_create(&mdns_t, 0, mdns, &s) != 0) syserr("pthread_create error");

  pthread_t udp_client_t;
  if (pthread_create(&udp_client_t, 0, udp_client, &s) != 0) syserr("pthread_create error");

  // pthread_t udp_client_t;
  // if (pthread_create(&udp_client_t, 0, udp_client, &s) != 0) {
  //   perror("pthread_create error");
  //   exit (__LINE__);
  // }
  // pthread_detach(udp_client_t);
  


  // create_thread(mdns_t, mdns);
  // create_thread(udp_client_t, udp_client);
  pthread_join(mdns_t, NULL);
  return 0;
}