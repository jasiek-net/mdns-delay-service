#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

struct host_data {
	uint32_t ip;
    struct sockaddr addr_udp;
    struct sockaddr addr_icm;
    struct sockaddr addr_tcp;
	uint64_t udp[10],
			 tcp[10],
			 icm[10];
	int	u, t, i;
	int tcp_numb,
		icm_seq;
	uint64_t tcp_time,
			 icm_time;
	int is_tcp, is_udp, is_icm;
	int check;
};

typedef struct host_data stack_data; //stack data type - set for integers, modifiable
typedef struct stack Node; //short name for the stack type
struct stack { //stack structure format
    stack_data host;
    Node *next;
};

int stack_len(); //stack length
void stack_push(struct sockaddr addr); //pushes a value d onto the stack
void stack_print(); //prints all the stack data
void stack_clear(); //clears the stack of all elements
void stack_check();
//void stack_snoc(Node **node_head, stack_data d); //appends a node
void add_udp_measurement(struct sockaddr *sa, uint64_t result);
void add_icm_measurement(struct sockaddr *sa, uint64_t result);
void add_tcp_measurement(int numb, uint64_t end);
void create_or_add(uint32_t ip, char *type);

#endif
