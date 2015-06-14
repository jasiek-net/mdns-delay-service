/* THREADS.C and some helpful function */
#ifndef THREADS_H
#define THREADS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>

#define UDP_SRV_PORT 3382

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

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

struct host_data {
	uint32_t ip;
    struct sockaddr addr_udp;
    struct sockaddr addr_tcp;
	int udp[10],
		tcp[10],
		icm[10],
		u, t, i;
	int tcp_numb,
		tcp_serv,
		icm_seq;
	uint64_t tcp_time,
			 icm_time;
	int is_tcp, is_udp;
};

typedef struct host_data stack_data; //stack data type - set for integers, modifiable
typedef struct stack Node; //short name for the stack type
struct stack { //stack structure format
    stack_data host;
    Node *next;
};

Node *head; //pointer to stack head
pthread_rwlock_t lock;
pthread_rwlock_t lock_tcp;
pthread_rwlock_t lock_telnet;
socklen_t addr_len;
int udp_port;
int tcp_port;
int telnet_port;
int measure_delay;
int telnet_delay;
int mdns_delay;
int ssh_multicast;


// MAX TCP OPEN SOCKETS
#define MAX_SERVERS 1019



int stack_len(); //stack length
void stack_push(struct sockaddr addr); //pushes a value d onto the stack
stack_data stack_pop(); //removes the head from the stack & returns its value
void stack_print(); //prints all the stack data
void stack_clear(); //clears the stack of all elements
//void stack_snoc(Node **node_head, stack_data d); //appends a node
int stack_elem(struct sockaddr *sa); //checks for an element
void add_measurement(struct sockaddr *sa, char *type, int result);
void add_tcp(int numb, uint64_t end);
void create_or_add(uint32_t ip, char *type);

#endif

/* ERROR.C */
#ifndef _ERR_
#define _ERR_

/* wypisuje informacje o blednym zakonczeniu funkcji systemowej 
i konczy dzialanie */
extern void syserr(const char *fmt, ...);

/* wypisuje informacje o bledzie i konczy dzialanie */
extern void fatal(const char *fmt, ...);

#endif



