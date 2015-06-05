
/* THREADS.C and some helpful function */
#ifndef THREADS_H
#define THREADS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>

#define UDP_SRV_PORT 3382

extern void *mdns(void * arg);
extern uint64_t gettime();
extern void print_ip_port(struct sockaddr sa);
extern void *udp_client(void *arg);
extern void *udp_server(void *arg);

#endif

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

struct host_data {
    struct sockaddr addr;
	int udp[10],
		tcp[10],
		icm[10],
		u, t, i;
};

typedef struct host_data stack_data; //stack data type - set for integers, modifiable
typedef struct stack Node; //short name for the stack type
struct stack { //stack structure format
    stack_data host;
    Node *next;
};

struct stuff {
  pthread_rwlock_t lock; // dafaq? wut? why?
  Node *head; //pointer to stack head
};

int stack_len(Node *node_head); //stack length
void stack_push(Node **node_head, struct sockaddr addr); //pushes a value d onto the stack
stack_data stack_pop(Node **node_head); //removes the head from the stack & returns its value
void stack_print(Node **node_head); //prints all the stack data
void stack_clear(Node **node_head); //clears the stack of all elements
//void stack_snoc(Node **node_head, stack_data d); //appends a node
int stack_elem(Node **node_head, struct sockaddr *sa); //checks for an element
int add_measurement(Node **node_head, struct sockaddr *sa, char *type, int result);

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



