#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "threads.h"

int stack_len(Node *node_head) {
    Node *curr = node_head;
    int len = 0;
     
    while(curr) {
        ++len;
        curr = curr -> next;
    }
    return len;
}

void init_host(struct host_data h, struct sockaddr addr) {
} 

void stack_push(Node **node_head, struct sockaddr addr) {
    Node *node_new = malloc(sizeof(Node));

	node_new->host.addr = addr;
	node_new->host.u = node_new->host.t = node_new->host.i = 0;
	memset(node_new->host.udp, 0, sizeof(node_new->host.udp));
	memset(node_new->host.tcp, 0, sizeof(node_new->host.tcp));
	memset(node_new->host.icm, 0, sizeof(node_new->host.icm));
    node_new -> next = *node_head;
    *node_head = node_new;
}
 
stack_data stack_pop(Node **node_head) {
    Node *node_togo = *node_head;
    stack_data d;
     
    if(node_head) {
        d = node_togo -> host;
        *node_head = node_togo -> next;
        free(node_togo);
    }
    return d;
}
 
void stack_print(Node **node_head) {
    Node *ptr = *node_head;
     
    if(!ptr)
        puts("the stack is empty");
    else {
        while(ptr) {
			print_ip_port(ptr->host.addr);
            ptr = ptr -> next;
        }
        putchar('\n');
    }
}
 
void stack_clear(Node **node_head) {
    while(*node_head)
        stack_pop(node_head);
}
 
// void stack_snoc(Node **node_head, stack_data d) {
//     Node *ptr = *node_head;
     
//     if(!ptr) stack_push(node_head, d);
//     else {
//         //find the last node
//         while(ptr -> next)
//             ptr = ptr -> next;
//         //build the node after it
//         stack_push(&(ptr -> next), d);
//     }
// }

int stack_elem(Node **node_head, struct sockaddr *sa) {
    Node *ptr = *node_head;
     
    while(ptr) {
        if(strcmp(((struct sockaddr *) sa)->sa_data,  ((struct sockaddr *) &ptr->host.addr)->sa_data)) //set for numbers, modifiable
            ptr = ptr->next;
        else {
            return 1;
        }
    }
    return 0;
}

int add_measurement(Node **node_head, struct sockaddr *sa, char *type, int result) {
    Node *p = *node_head;
    
    while(p) {
        if(strcmp(sa->sa_data, p->host.addr.sa_data)) //set for numbers, modifiable
            p = p->next;
        else {
			if (!strcmp("udp", type)) {
				printf("pomiar: %d, wynik: %d\n", p->host.u, result); 
				p->host.udp[ p->host.u ] = result;
				p->host.u = (p->host.u+1)%10;
			} else if (!strcmp("tcp", type)) {
				p->host.tcp[ p->host.t ] = result;
				p->host.t = (p->host.t+1)%10;
			} else if (!strcmp("icm", type)) {
				p->host.icm[ p->host.i ] = result;
				p->host.i = (p->host.i+1)%10;
			}
			return 1;
        }
    }
    return 0;
}
