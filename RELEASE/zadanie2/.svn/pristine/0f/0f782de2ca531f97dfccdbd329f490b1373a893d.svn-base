#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <poll.h>
#include <netdb.h>

#include "linkedlist.h"
#include "opoznienia.h"
#include "err.h"

#define TRUE 1
#define FALSE 0
#define BUF_SIZE 1927 // 80 x 24 + 7 (for clear)

struct result {
  int udp, tcp, icm, ave;
  char ip[15];
};

int comp(const void * elem1, const void * elem2) {
    struct result f = *((struct result *) elem1);
    struct result s = *((struct result *)elem2);
    if (f.ave > s.ave) return  1;
    if (f.ave < s.ave) return -1;
    return 0;
}

int ave(uint64_t *t) {
  int i, d = 10;
  uint64_t k = 0;
  for (i = 0; i < 10; i++) {
    if (t[i] == 0) d--;
    else k += t[i];
  }
  if (k == 0) return 0;
  else        return k/d;
}

char* itoa(int val){
  if (val == 0) return "XXX";
  int base = 10;
  static char buf[32] = {0};
  int i = 30;
  for(; val && i ; --i, val /= base)
    buf[i] = "0123456789abcdef"[val % base];
  return &buf[i+1];
}

int size_int(int x) {
  int i = 1;
  if (x == 0) return 3;
  while (x /= 10)
    i++;
  return i;
}

char *msg;
int msg_size;

void *telnet_message(void *arg) {

  while(1) {
    int size = stack_len();
    struct result tab[size];

    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");

    Node *p = head;
    int i = 0;
    while(p) {
      // tab[i].ip = memset(strlen(inet_ntoa(((struct sockaddr_in *) &p->host.addr_icm)->sin_addr)));
      strcpy(tab[i].ip, inet_ntoa(((struct sockaddr_in *) &p->host.addr_icm)->sin_addr));
      tab[i].udp = ave( p->host.udp );
      tab[i].tcp = ave( p->host.tcp );
      tab[i].icm = ave( p->host.icm );
      if (!tab[i].udp && !tab[i].tcp && !tab[i].icm)
        tab[i].ave = 0;
      else
        tab[i].ave = (tab[i].udp + tab[i].tcp + tab[i].icm)/3;
      p = p->next;
      i++;
    }

    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");

    qsort(tab, sizeof(tab) / sizeof(*tab), sizeof(*tab), comp);

    if (pthread_rwlock_rdlock(&lock_telnet) != 0) syserr("pthread_rwlock_rdlock error");

    free(msg);
    int off, offright, offave;
    if (size > 50) offave = 50;
    else offave = size;
    msg_size = size < 24 ? BUF_SIZE-7 : (size * 80);
    msg = malloc(msg_size * sizeof(char));
    memset(msg, ' ', msg_size);
    off = 0;
    offright = 80;
    for (i = 0; i < size; i++) {
      memcpy((char *)(msg + off), tab[i].ip, strlen(tab[i].ip));
      off += 16;
      off += offave;
      memcpy((char *)(msg + off), itoa(tab[i].udp), size_int(tab[i].udp));
      // printf("udp: %s\n", itoa(tab[i].udp));
      off += size_int(tab[i].udp) + 1;
      memcpy((char *)(msg + off), itoa(tab[i].tcp), size_int(tab[i].tcp));
      // printf("tcp: %s\n", itoa(tab[i].tcp));
      off += size_int(tab[i].tcp) + 1;
      memcpy((char *)(msg + off), itoa(tab[i].icm), size_int(tab[i].icm));
      // printf("icm: %s\n", itoa(tab[i].icm));
      off = offright;
      offright += 80;
      if (offave) offave--;
    }

    if (pthread_rwlock_unlock(&lock_telnet) != 0) syserr("pthred_rwlock_unlock error");

    sleep(telnet_delay);
  }
}


void *telnet(void *arg) {

 pthread_t telnet_message_t;
 if (pthread_create(&telnet_message_t, 0, telnet_message, NULL) != 0) syserr("pthread_create");

  struct pollfd client[MAX_SERVERS];
  int offset[MAX_SERVERS];
  struct sockaddr server;
  char buf[BUF_SIZE];
  ssize_t rval;
  int msgsock, activeClients, i, ret;

  for (i = 0; i < MAX_SERVERS; ++i) {
    client[i].fd = -1;
    client[i].events = POLLIN;
    client[i].revents = 0;
    offset[i] = 0;
  }
  activeClients = 0;

  client[0].fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client[0].fd < 0) syserr("socket");

  ((struct sockaddr_in *) &server)->sin_family = AF_INET;
  ((struct sockaddr_in *) &server)->sin_addr.s_addr = htonl(INADDR_ANY);
  ((struct sockaddr_in *) &server)->sin_port = htons(telnet_port);

  if (bind(client[0].fd, &server, addr_len) < 0) syserr("bind");

  // printf("telnet:      ");
  // print_ip_port(server);

  if (listen(client[0].fd, 5) == -1) syserr("listen");

  while(1) {

    for (i = 0; i < MAX_SERVERS; ++i) client[i].revents = 0;

    ret = poll(client, MAX_SERVERS, telnet_delay*1000);
    if (ret < 0) syserr("poll");
    else if (ret == 0) {
      // printf("waiting...\n");
      for (i = 1; i < MAX_SERVERS; ++i) {
        if (client[i].fd != -1) {
          int size = 0;
          if (msg_size - offset[i] > BUF_SIZE-7)
            size = BUF_SIZE-7;
          else
          if (msg_size - offset[i] > 0)
            size = msg_size - offset[i];
          memset(buf, ' ', BUF_SIZE);
          memcpy(buf, "\033[2J\033[H", 7);
          memcpy(buf + 7, msg + offset[i], size);
          rval = write(client[i].fd, buf, BUF_SIZE);
          if (rval < 0) {
            perror("Writing stream message");
            if (close(client[i].fd) < 0) perror("close");
            client[i].fd = -1;
            activeClients -= 1;
          }
        }
      }
    } else {  // ret > 0 
      if (client[0].revents & POLLIN) {
        msgsock = accept(client[0].fd, (struct sockaddr*)0, (socklen_t*)0);
        if (msgsock == -1) syserr("accept");
        else {
          for (i = 1; i < MAX_SERVERS; ++i) {
            if (client[i].fd == -1) {
              client[i].fd = msgsock;
              ret = snprintf(buf, sizeof(buf), "\377\373\003\377\373\001");
              if (ret != 6) syserr("snprintf");
              rval = write(client[i].fd, buf, ret);
              if (rval < 0) {
                perror("Writing stream message");
                if (close(client[i].fd) < 0) perror("close");
                client[i].fd = -1;
                activeClients -= 1;
              }
              activeClients += 1;
              break;
            }
          }
          if (i >= MAX_SERVERS) {
            fprintf(stderr, "Too many telnet clients!\n");
            if (close(msgsock) < 0) syserr("close");
          }
        }
      }

      // czytanie wiadomości: znaków Q lub A
      for (i = 1; i < MAX_SERVERS; ++i) {
        if (client[i].fd != -1 && (client[i].revents & (POLLIN | POLLERR))) {
          rval = read(client[i].fd, buf, BUF_SIZE);
          if (rval < 0) {
            perror("Reading stream message");
            if (close(client[i].fd) < 0)
              perror("close");
            client[i].fd = -1;
            activeClients -= 1;
          } else if (rval == 0) {
            fprintf(stderr, "Ending connection\n");
            if (close(client[i].fd) < 0)
              perror("close");
            client[i].fd = -1;
            activeClients -= 1;
          }
          else {
            if (buf[0] == 'A') {
              // printf("dół!\n");
              offset[i] += 80;
            }
            if (buf[0] == 'Q') {
              // printf("góra!\n");
              if (offset[i])
                offset[i] -= 80;
            }
            int size = 0;
            if (msg_size - offset[i] > BUF_SIZE-7)
              size = BUF_SIZE-7;
            else
            if (msg_size - offset[i] > 0)
              size = msg_size - offset[i];
            memset(buf, ' ', BUF_SIZE);
            memcpy(buf, "\033[2J\033[H", 7);
            memcpy(buf+7, msg+offset[i], size);
            rval = write(client[i].fd, buf, BUF_SIZE);
            if (rval < 0) {
              perror("Writing stream message");
              if (close(client[i].fd) < 0) perror("close");
              client[i].fd = -1;
              activeClients -= 1;
            }
          }
        }
      }
    }
  }
}