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
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define BUF_SIZE 1927 // 80 x 24 + 7 (for clear)

static int finish = FALSE;

#include "threads.h"

#define IAC_WILL_SGA        "\377\373\003"
#define IAC_WILL_ECHO       "\377\373\001"
#define CLR_SCR             "\033[2J\033[H"
#define RED_LETTERS         "\033[31;7m"

#define IAC     "\377"
#define DONT    "\376"
#define DO      "\375"
#define WONT    "\374"
#define WILL    "\373"
#define SGA     "\003"
#define ECHO    "\001"

/* ObsĹuga sygnaĹu koĹczenia */
  // static void catch_int (int sig) {
  //   finish = TRUE;
  //   fprintf(stderr,
  //           "Signal %d catched. No new connections will be accepted.\n", sig);
  // }

//int main(int argc, char* argv[]) 
// {
//     int x[] = {4,5,2,3,1,0,9,8,6,7};

//     qsort (x, sizeof(x)/sizeof(*x), sizeof(*x), comp);

//     for (int i = 0 ; i < 10 ; i++)
//         printf ("%d ", x[i]);

//     return 0;
// }

int ave(int *t) {
  int i, k = 0, d = 10;
  for (i = 0; i < 10; i++) {
    if (t[i] == 0) d--;
    else k += t[i];
  }
  if (k == 0) return 0;
  else        return k/d;
}

  struct result {
    int udp, tcp, icm, ave;
    char *ip;
  };

int comp(const void * elem1, const void * elem2) {
    struct result f = *((struct result *) elem1);
    struct result s = *((struct result *)elem2);
    if (f.ave > s.ave) return  1;
    if (f.ave < s.ave) return -1;
    return 0;
}

char *msg;

#define MSG_SIZE 1927
char* itoa(int val){
  if (val == 0) return "xxx";
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

void *telnet_message(void *arg) {

  itoa(0);
  while(1) {
    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");
    if (pthread_rwlock_rdlock(&lock_telnet) != 0) syserr("pthread_rwlock_rdlock error");
    int i = 0;
    int size = stack_len();
    Node *p = head;
    struct result tab[size];
    while(p) {
      tab[i].ip = inet_ntoa(((struct sockaddr_in *) &p->host.addr)->sin_addr);
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
    qsort(tab, sizeof(tab) / sizeof(*tab), sizeof(*tab), comp);
    free(msg);
    // zawsze o jeden więcej (żeby na końcu był znak '\0')
    int off = 0;
    int msg_size = size * 80;
    msg = malloc(msg_size);
    memset(msg, '-', msg_size);
//    msg[msg_size - 1] = '\0';
    memcpy((char *)(msg + off), "\033[2J\033[H", 7);
    off = off + 7;
    for (i = 0; i < size; i++) {
      memcpy((char *)(msg + off), tab[i].ip, sizeof(tab[i].ip)+1);
      off = off + 16;
      memcpy((char *)(msg + off), itoa(tab[i].udp), size_int(tab[i].udp));
//      off = off + size_int(tab[i].udp) + 1;
      off += 4;
      memcpy((char *)(msg + off), itoa(tab[i].tcp), size_int(tab[i].tcp));
//      off = off + size_int(tab[i].tcp) + 1;
      off += 4;
      memcpy((char *)(msg + off), itoa(tab[i].icm), size_int(tab[i].icm));
      off += 52;
//      msg[off-1] = '\n';
//      off = off + size_int(tab[i].icm) + 1;

      // printf("itoa: %s\n", itoa(tab[i].udp));
      // printf("size: %d\n", size_int(tab[i].udp));

      // printf("sizeof(tab[i].ip) %d\n", (int) sizeof(tab[i].ip));
      // snprintf(((char *) msg + off), 15, tab[i].ip); // for ip
      // off = off + 79;
      // snprintf(((char *) msg + off), 1, "\n"); // for ip

//  data_len = snprintf(((char*) send_buffer + ICMP_HEADER_LEN), sizeof(send_buffer) - ICMP_HEADER_LEN, "1\007T\004");

    }
    // printf("%s\n", msg);
    if (pthread_rwlock_unlock(&lock_telnet) != 0) syserr("pthred_rwlock_unlock error");
    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");
    sleep(telnet_delay);
  }

}


void *telnet(void *arg) {

 pthread_t telnet_message_t;
 if (pthread_create(&telnet_message_t, 0, telnet_message, NULL) != 0) syserr("pthread_create");

  // /* Po Ctrl-C koĹczymy */
  // if (signal(SIGINT, catch_int) == SIG_ERR) {
  //   perror("Unable to change signal handler\n");
  //   exit(EXIT_FAILURE);
  // }

  /* Inicjujemy tablicÄ z gniazdkami klientĂłw, client[0] to gniazdko centrali */

  struct pollfd client[MAX_SERVERS];
  int offset[MAX_SERVERS];
  struct sockaddr server;
  char buf[BUF_SIZE];
  size_t length;
  ssize_t rval;
  int msgsock, activeClients, i, ret;

  for (i = 0; i < MAX_SERVERS; ++i) {
    client[i].fd = -1;
    client[i].events = POLLIN;
    client[i].revents = 0;
    offset[i] = 0;
  }
  activeClients = 0;

  /* Tworzymy gniazdko centrali */
  client[0].fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client[0].fd < 0) syserr("socket");

  ((struct sockaddr_in *) &server)->sin_family = AF_INET;
  ((struct sockaddr_in *) &server)->sin_addr.s_addr = htonl(INADDR_ANY);
  ((struct sockaddr_in *) &server)->sin_port = htons(telnet_port);

  if (bind(client[0].fd, &server, addr_len) < 0) syserr("bind");

  printf("TELNER WORKIN ON: ");
  print_ip_port(server);

  /* Zapraszamy klientĂłw */
  if (listen(client[0].fd, 5) == -1) syserr("listen");

  char clear[7];
  snprintf(clear, 7, "\033[2J\033[H");
  /* Do pracy */
  while(1) {

    for (i = 0; i < MAX_SERVERS; ++i) client[i].revents = 0;

    ret = poll(client, MAX_SERVERS, telnet_delay*1000);
    if (ret < 0) syserr("poll");
    else if (ret == 0) {
      printf("waiting...\n");
      for (i = 1; i < MAX_SERVERS; ++i) {
        if (client[i].fd != -1) {
      //    rval = write(client[i].fd, clear, 7);         
          rval = write(client[i].fd, msg, MSG_SIZE);
        //   if (rval < 0) {
        //     perror("Writing stream message");
        //     if (close(client[i].fd) < 0) perror("close");
        //     client[i].fd = -1;
        //     activeClients -= 1;
        //   }
        }
      }
    } else {  // ret > 0 
      if (finish == FALSE && (client[0].revents & POLLIN)) {
        msgsock = accept(client[0].fd, (struct sockaddr*)0, (socklen_t*)0);
        if (msgsock == -1) syserr("accept");
        else {
          for (i = 1; i < MAX_SERVERS; ++i) {
            if (client[i].fd == -1) {
              client[i].fd = msgsock;
//                                               IAC WILL SGA IAC WILL ECHO 
              ret = snprintf(buf, sizeof(buf), "\377\373\003\377\373\001");
              if (ret != 6) syserr("snprintf");
              write(client[i].fd, buf, ret);
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

      // #define CLR_SCR             "\033[2J\033[H"
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
            printf("-->%.*s\n", (int)rval, buf);
            if (buf[0] == 'A') {
              printf("dół!\n");
              offset[i]++;

            }
            if (buf[0] == 'Q') {
              printf("góra!\n");
              offset[i]--;
            }

          }
        }
      }
    }
  }

  if (client[0].fd >= 0)
    if (close(client[0].fd) < 0)
      perror("Closing main socket");
  exit(EXIT_SUCCESS);





int listenfd = 0, connfd = 0;
struct sockaddr_in serv_addr, cli_addr;
int clilen;

char sendBuff[1025];
time_t ticks;

listenfd = socket(AF_INET, SOCK_STREAM, 0);
memset(&serv_addr, '0', sizeof(serv_addr));
memset(sendBuff, '0', sizeof(sendBuff));

serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
serv_addr.sin_port = htons(5000); // listen on port 5000

bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

listen(listenfd, 10);
clilen=sizeof(cli_addr);
while(1)
{
    connfd = accept(listenfd, (struct sockaddr*) &cli_addr, &clilen);

    char bafer[4096];

// #define IAC     "\377"
// #define DONT    "\376"
// #define DO      "\375"
// #define WONT    "\374"
// #define WILL    "\373"
// #define SGA     "\003"
// #define ECHO    "\001"


    write (connfd, IAC, 1);
    write (connfd, WILL, 1);
    write (connfd, SGA, 1);
    recv (connfd, bafer, 3, MSG_WAITALL);
    int i;
    for (i=0;i<strlen(bafer);i++)
        switch ((unsigned char)bafer[i]) {
            case 255: printf("IAC\n");break;
            case 254: printf("DONT\n");break;
            case 253: printf("DO\n");break;
            case 252: printf("WONT\n");break;
            case 251: printf("WILL\n");break;
            case 3: printf("SUPPRESS-GO-AHEAD\n");break;
            case 1: printf("ECHO\n");break;
        }

    write (connfd, IAC, 1);
    write (connfd, WILL, 1);
    write (connfd, ECHO, 1);
    recv (connfd, bafer, 3, MSG_WAITALL);
    for (i=0;i<strlen(bafer);i++)
        switch ((unsigned char)bafer[i]) {
            case 255: printf("IAC\n");break;
            case 254: printf("DONT\n");break;
            case 253: printf("DO\n");break;
            case 252: printf("WONT\n");break;
            case 251: printf("WILL\n");break;
            case 3: printf("SUPPRESS-GO-AHEAD\n");break;
            case 1: printf("ECHO\n");break;
        }

    write(connfd, CLR_SCR, 7);
    write(connfd, RED_LETTERS, 7);
    write (connfd, "Enter key: ", 11);

do {
    recv (connfd,bafer,1,MSG_WAITALL);
    write(connfd, bafer, 1);
    printf("%d\n",(unsigned char)bafer[0]);
} while (bafer[0]!='I');
    write (connfd, "\n",1);
    write(connfd, "\033[0m", strlen("\033[0m")); // reset color
    close(connfd);
 }
}