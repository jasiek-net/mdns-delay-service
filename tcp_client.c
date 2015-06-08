
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

#include <poll.h>

#include "threads.h"

#include <fcntl.h>

#define MAX_SERVERS 1019

//struct pollfd server[MAX_SERVERS];


// Pola:
// fd - numer obserwowanego deskryptora
// events - flagi obserwowane zdarzenia
// revents - flagi zdarzeń, które zaszły w czasie wywołania poll()
// POLLIN - obserwowanie nadejścia danych do odczytu
// POLLOUT - obserwowanie możliwości zapisania danych
// POLLPRI - obserwowanie nadejścia danych wyjątkowych (out-of-band)

	  // struct addrinfo addr_hints, *addr_1;
	  // memset(&addr_hints, 0, sizeof(struct addrinfo));
	  // addr_hints.ai_flags = 0;
	  // addr_hints.ai_family = AF_INET;
	  // addr_hints.ai_socktype = SOCK_STREAM;
	  // addr_hints.ai_protocol = IPPROTO_TCP;
	  // rc =  getaddrinfo("localhost", "4242", &addr_hints, &addr_1);
	  // if (rc != 0) syserr("getaddrinfo: %s\n", gai_strerror(rc));
	  // printf("new server: ");
	  // print_ip_port(*(addr_1->ai_addr));

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

pthread_rwlock_t lock_tcp;


void *tcp_client_connect(void *arg) {
  socklen_t addr_len = sizeof(struct sockaddr);
	int sec = *(int *) arg;
	int sock, rc, i;
	uint64_t start;

	while(1) {
    if (pthread_rwlock_rdlock(&lock_tcp) != 0) syserr("pthread_rwlock_rdlock error");

    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");

    Node *p = head;
    start = gettime();
    while(p) {
    	if (p->host.tcp_serv && p->host.tcp_time == 0) {
  			sock = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
				if (sock < 0) syserr("socket");

				i = get_free_server();
			  if (i == -1) printf("To many TCP servers!\n");
			  else {
				  server[i].fd = sock;
				  server[i].events = POLLOUT;
				  server[i].revents = 0;

					rc = connect(sock, &p->host.addr, addr_len); 
				  if (rc < 0 && errno != EINPROGRESS) syserr("connect");
			  }  		
			  p->host.tcp_numb = i;
			  p->host.tcp_time = start;
    	}
		  p = p->next;
		}

    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");

    if (pthread_rwlock_unlock(&lock_tcp) != 0) syserr("pthred_rwlock_unlock error");

		sleep(sec);
	}
}

void *tcp_client (void *arg) {
	int sec = *(int *) arg;
	if (pthread_rwlock_init(&lock_tcp, NULL) != 0) syserr("pthread_rwlock_init");

	init_servers();

  pthread_t tcp_client_connect_t;
  if (pthread_create(&tcp_client_connect_t, 0, tcp_client_connect, &sec) != 0) syserr("pthread_create");

  // printf("POLLOUT: %d\n", POLLOUT);
  // printf("POLLIN %d\n", POLLIN);
  // printf("POLLERR %d\n", POLLERR);
  // printf("POLLNVAL %d\n", POLLNVAL);
  // printf("POLLPRI %d\n", POLLPRI);
  // printf("POLLHUP %d\n", POLLHUP);

	int rc, valopt, i;
	socklen_t lon = sizeof(int); 

	while(1) {

	    if (pthread_rwlock_rdlock(&lock_tcp) != 0) syserr("pthread_rwlock_rdlock error");
			rc = poll(server, 1, sec*1000);
		  if (rc < 0)
		  	syserr("poll");
	  	if (rc == 0) {
	  		printf("timeout!\n");
	  	} else {
	  		for (i = 0; i < MAX_SERVERS; i++) {
			  	if (server[i].revents & POLLOUT) {
			      if (getsockopt(server[i].fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) syserr("getsockopt");
			      if (valopt) {
			      	printf("Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
			      	add_tcp(i, 0); 
			      } else {
			      	printf("połączono;)\n");
			      	add_tcp(i, gettime());
			      }
			      if (close(server[i].fd) < 0) syserr("close");
				    server[i].fd = -1;
				    server[i].revents = 0;
			  	}  			
	  		}
	  	}
	    
	  if (pthread_rwlock_unlock(&lock_tcp) != 0) syserr("pthred_rwlock_unlock error");
	}
	return NULL;
}

// if (client[0].fd >= 0)
//   if (close(client[0].fd) < 0)
//     perror("Closing main socket");
//   exit(EXIT_SUCCESS);


void connect_w_to(void) { 
  int res; 
  struct sockaddr_in addr; 
  long arg; 
  fd_set myset; 
  struct timeval tv; 
  int valopt; 
  socklen_t lon; 

  // Create socket 
  int soc = socket(AF_INET, SOCK_STREAM, 0); 
  if (soc < 0) { 
     fprintf(stderr, "Error creating socket (%d %s)\n", errno, strerror(errno)); 
     exit(0); 
  } 

  addr.sin_family = AF_INET; 
  addr.sin_port = htons(2000); 
  addr.sin_addr.s_addr = inet_addr("192.168.0.1"); 

  // Set non-blocking 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg |= O_NONBLOCK; 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // Trying to connect with timeout 
  res = connect(soc, (struct sockaddr *)&addr, sizeof(addr)); 
  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); 
        do { 
           tv.tv_sec = 15; 
           tv.tv_usec = 0; 
           FD_ZERO(&myset); 
           FD_SET(soc, &myset); 
           res = select(soc+1, NULL, &myset, NULL, &tv); 
           if (res < 0 && errno != EINTR) { 
              fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
              exit(0); 
           } 
           else if (res > 0) { 
              // Socket selected for write 
              lon = sizeof(int); 
              if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
                 fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
                 exit(0); 
              } 
              // Check the value returned... 
              if (valopt) { 
                 fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt) ); 
                 exit(0); 
              } 
              break; 
           } 
           else { 
              fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
              exit(0); 
           } 
        } while (1); 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        exit(0); 
     } 
  } 
  // Set to blocking mode again... 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg &= (~O_NONBLOCK); 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // I hope that is all 
}





// void *tcp_client (void *arg) {
// 	printf("tcp_client\n");
// 	struct pollfd servers[MAX_SERVERS];


// 	int sock, rc;
// //	long opt;

//   struct addrinfo addr_hints, *addr_1, *addr_2;
//   memset(&addr_hints, 0, sizeof(struct addrinfo));
//   addr_hints.ai_flags = 0;
//   addr_hints.ai_family = AF_INET;
//   addr_hints.ai_socktype = SOCK_STREAM;
//   addr_hints.ai_protocol = IPPROTO_TCP;
//   rc =  getaddrinfo("localhost", "4242", &addr_hints, &addr_1);
//   if (rc != 0) syserr("getaddrinfo: %s\n", gai_strerror(rc));
//   addr_hints.ai_flags = 0;
//   addr_hints.ai_family = AF_INET;
//   addr_hints.ai_socktype = SOCK_STREAM;
//   addr_hints.ai_protocol = IPPROTO_TCP;
//   rc =  getaddrinfo("localhost", "2424", &addr_hints, &addr_2);
//   if (rc != 0) syserr("getaddrinfo: %s\n", gai_strerror(rc));

//   // if((opt = fcntl(sock, F_GETFL, NULL)) < 0) syserr("fcntl"); 
//   // opt |= O_NONBLOCK; 
//   // if(fcntl(sock, F_SETFL, opt) < 0) syserr("fcntl");

//   // while(1) {
//   int i;
//   for (i = 0; i < MAX_SERVERS; i++) {
// 	  sock = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
// 	  if (sock < 0) syserr("socket");

// 	}

// 	  sock = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
// 	  if (sock < 0) syserr("socket");

// 	  rc = connect(sock, addr_1->ai_addr, addr_1->ai_addrlen); 
// 	  if (rc < 0) {
// 	  	if (errno == EINPROGRESS) printf("EINPROGRESS in connect() - selecting\n"); 
// 			else printf("nie ma takiego hosta\n"); //syserr("connect");	  	
// 	  } else {
// 	  	printf("połączono!\n");
// 	  }

// 	  rc = connect(sock, addr_2->ai_addr, addr_2->ai_addrlen); 
// 	  if (rc < 0) {
// 	  	if (errno == EINPROGRESS) printf("EINPROGRESS in connect() - selecting\n"); 
// 			else syserr("connect");	  	
// 	  } else {
// 	  	printf("połączono!\n");
// 	  }


// 	  printf("tcp śpi...\n");
// 	  sleep(1);
// //	  close(sock);
// //	}

//   return 0;
// }







	// socklen_t addr_len = sizeof(struct sockaddr);
	// uint64_t start;
	// int sock, err, i;

	// init_servers();

	// while(1) {

 //    if (pthread_rwlock_rdlock(&lock) != 0) syserr("pthread_rwlock_rdlock error");
 
 //    Node *p = head;
 //    start = gettime();
 //    while(p) {
 //    	if (!p->host.tcp_serv) {
	//     	sock = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	// 	  	if (sock < 0) syserr("socket");
	// 		  err = connect(sock, &p->host.addr, addr_len);
	// 		  if (err < 0) {
	// 		  	if (errno == EINPROGRESS) {
	// 		  		printf("EINPROGRESS in connect() - polling\n");
	// 		  		i = get_free_server();
	// 		  		if (i == -1)
	// 		  			fprintf(stderr, "Too many servers for TCP measurment!\n");
	// 		  		else {
	// 			  		server[i].fd = sock;
	// 			  		p->host.tcp_serv = i;
	// 						p->host.tcp_time = start;			  			
	// 		  		}
	// 		  	} else syserr("connect");	  	
	// 		  } else printf("bardzo niedobrze: połączono!\n");
	//       print_ip_port(p->host.addr);
 //    	}
	//     p = p->next;
 //    }
 
 //    if (pthread_rwlock_unlock(&lock) != 0) syserr("pthred_rwlock_unlock error");

	// 	int milisec = sec*1000;
	// 	while(milisec > 0) {
	// 		// petla zerujace revents lub obsługa zakończenia!
	// 	  for (i = 0; i < MAX_SERVERS; i++) server[i].revents = 0;

	// 		uint64_t poll_start, poll_end;
	// 	 	poll_start = gettime();
	// 	 	err = poll(server, MAX_SERVERS, milisec);
	// 	  if (err < 0) perror("poll");
	// 	  else if (err > 0) {
	// 	    for (i = 0; i < MAX_SERVERS; i++) {
	// 	      if (server[i].fd != -1 && (server[i].revents & POLLIN)) {
	// 	        if (close(server[i].fd) < 0) syserr("close");
	// 	        // add new measurment tcp to server
	// 	        printf("odebrane połączenie!\n");
	// 	        server[i].fd = -1;
	// 	      }
	// 	    }
	// 		  poll_end = gettime();
	// 		  milisec -= (int) (poll_end - poll_start);
	// 	  } else {
	// 	  	milisec = 0;
	// 	  	printf("timeout!\n");
	// 	  }
	// 	}
	// }
