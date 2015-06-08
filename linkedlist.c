#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "threads.h"

int stack_len() {
    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");

    Node *p = head;
    int len = 0;     
    while(p) {
        ++len;
        p = p->next;
    }

    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");
    return len;
}

void init_host(struct host_data * h, struct sockaddr addr) {
	(*h).addr = addr;
	(*h).u = (*h).t = (*h).i = 0;
	memset((*h).udp, 0, sizeof((*h).udp));
	memset((*h).tcp, 0, sizeof((*h).tcp));
	memset((*h).icm, 0, sizeof((*h).icm));
    (*h).tcp_time = 0;
    (*h).tcp_serv = 1;
    (*h).tcp_numb = 0;
    (*h).icm_time = 0;
    (*h).icm_seq = 0;

} 

void stack_push(struct sockaddr addr) {
  Node *new = malloc(sizeof(Node));
  init_host(&(new->host), addr);
	if (pthread_rwlock_wrlock(&lock) != 0) syserr("pthread_rwlock_wrlock error");
  new -> next = head;
  head = new;
  if (pthread_rwlock_unlock(&lock) != 0) syserr("pthread_rwlock_unlock error");
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

void stack_print() {
    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");

    Node *p = head;
    if(!p) printf("the stack is empty");
    else {
        while(p) {
            (void) printf("%s", inet_ntoa(((struct sockaddr_in *) &p->host.addr)->sin_addr));
			(void) printf(":");
			(void) printf("%d -> ", ntohs(((struct sockaddr_in *) &p->host.addr)->sin_port));
            p = p -> next;
        }
    }
    printf("\n");

    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");
}
 
void stack_clear(Node **node_head) {
    while(*node_head)
        stack_pop(node_head);
}
int stack_elem(struct sockaddr *sa) {
    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");
 
    Node *p = head;
     
    while(p) {
        if(strcmp(sa->sa_data, p->host.addr.sa_data)) //set for numbers, modifiable
            p = p->next;
        else {
            return 1;
        }
    }
    return 0;

   if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");
}

void add_measurement(struct sockaddr *sa, char *type, int result) {

	if (pthread_rwlock_wrlock(&lock) != 0) syserr("pthread_rwlock_wrlock error");

    Node *p = head;    
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
						
			break;
        }
    }
    
	if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");
}

void add_tcp(int numb, uint64_t end) {

  if (pthread_rwlock_wrlock(&lock) != 0) syserr("pthread_rwlock_wrlock error");

  Node *p = head;    
  while(p) {
    if (p->host.tcp_numb != numb)
      p = p->next;
    else {
      if (end == 0)
        p->host.tcp_time = 0;
      else {
        p->host.tcp[ p->host.t ] = (int) (end - p->host.tcp_time);
        printf("add tcp: %d\n", p->host.tcp[ p->host.t ]);
        p->host.t = (p->host.t + 1) % 10;
        p->host.tcp_time = 0;                
      }
      break;
    }
  }
  
  if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");
}
