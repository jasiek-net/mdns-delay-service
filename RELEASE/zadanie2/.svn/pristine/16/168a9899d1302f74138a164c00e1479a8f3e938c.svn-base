#ifndef OPOZNIENIA_H
#define OPOZNIENIA_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>

Node *head; //pointer to stack head
pthread_rwlock_t lock;
pthread_rwlock_t lock_telnet;
socklen_t addr_len;
struct sockaddr_in my_addr;

int udp_port;
int tcp_port;
int telnet_port;
int measure_delay;
int telnet_delay;
int mdns_delay;
int ssh_multicast;

#define MAX_SERVERS 1019	// max tcp open sockets

extern uint64_t gettime();
extern void print_ip_port(struct sockaddr sa);

extern void *udp_server(void *arg);
extern void *udp_client(void *arg);
extern void *tcp_client(void *arg);
extern void *icm_client(void *arg);
extern void *mdns(void *arg);
extern void *telnet(void *arg);

// for ICMP packets
unsigned short in_cksum(unsigned short *addr, int len);
void drop_to_nobody();

#endif