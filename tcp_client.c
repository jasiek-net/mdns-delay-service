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
#include <fcntl.h>
#include <poll.h>

#include "threads.h"

struct pollfd server[MAX_SERVERS];

void init_servers() {
	int i;
	for (i = 0; i < MAX_SERVERS; i++) {
		server[i].fd = -1;
		server[i].events = POLLOUT;	// we look up for tree-hand shake
		server[i].revents = 0;
	}
}

int get_free_server() {
	int i;
	for (i = 0; i < MAX_SERVERS; i++)
		if (server[i].fd == -1)
			return i;
	return -1;
}

void tcp_client_recieve() {
	int rc, valopt, i;
	socklen_t lon = sizeof(int); 
	int64_t delay = measure_delay*1000;
	uint64_t start, stop;
	while(1) {

			start = gettime();
			rc = poll(server, MAX_SERVERS, delay);
		  if (rc < 0) syserr("poll");
	  	if (rc == 0) {
	  		return;
	  	} else {
	  		for (i = 0; i < MAX_SERVERS; i++) {
			  	if (server[i].revents & POLLOUT) {
			      if (getsockopt(server[i].fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) syserr("getsockopt");
			      if (valopt) {
			      	// printf("Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
			      	add_tcp_measurement(i, 0);
			      } else {
			      	add_tcp_measurement(i, gettime());
			      }
			      if (close(server[i].fd) < 0) syserr("close");
				    server[i].fd = -1;
				    server[i].revents = 0;
			  	}  			
	  		}
	  	}
	  	stop = gettime();
	  	delay = delay - (stop - start);
			if (delay <= 0)
				return;
	}
}

void *tcp_client(void *arg) {
	init_servers();
	int sock, rc, i;

	while(1) {

    if (pthread_rwlock_wrlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");
    
    Node *p = head;
    while(p) {
    	if (p->host.is_tcp && !p->host.tcp_time) {
  			sock = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
				if (sock < 0) syserr("socket");

				i = get_free_server();
			  if (i == -1) printf("To many TCP servers!\n");
			  else {
				  server[i].fd = sock;
				  server[i].events = POLLOUT;
				  server[i].revents = 0;
					rc = connect(sock, &p->host.addr_tcp, addr_len); 
				  if (rc < 0 && errno != EINPROGRESS) syserr("connect");
			  }
			  p->host.tcp_numb = i;
			  p->host.tcp_time = gettime();
    	}
		  p = p->next;
		}
    
    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");

    tcp_client_recieve();

	}
}