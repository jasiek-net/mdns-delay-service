
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

#include "threads.h"

#include <fcntl.h>


void *tcp_client (void *arg) {
	printf("tcp_client\n");

	int sock, rc;
//	long opt;

  struct addrinfo addr_hints, *addr_1, *addr_2;
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_flags = 0;
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  rc =  getaddrinfo("localhost", "4242", &addr_hints, &addr_1);
  if (rc != 0) syserr("getaddrinfo: %s\n", gai_strerror(rc));
  addr_hints.ai_flags = 0;
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  rc =  getaddrinfo("localhost", "2424", &addr_hints, &addr_2);
  if (rc != 0) syserr("getaddrinfo: %s\n", gai_strerror(rc));

  // if((opt = fcntl(sock, F_GETFL, NULL)) < 0) syserr("fcntl"); 
  // opt |= O_NONBLOCK; 
  // if(fcntl(sock, F_SETFL, opt) < 0) syserr("fcntl");

  // while(1) {
  int i;
  for (i = 0; i < 1000; i++) {
	  sock = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	  if (sock < 0) syserr("socket");
	}

	  sock = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	  if (sock < 0) syserr("socket");

	  rc = connect(sock, addr_1->ai_addr, addr_1->ai_addrlen); 
	  if (rc < 0) {
	  	if (errno == EINPROGRESS) printf("EINPROGRESS in connect() - selecting\n"); 
			else printf("nie ma takiego hosta\n"); //syserr("connect");	  	
	  } else {
	  	printf("połączono!\n");
	  }

	  rc = connect(sock, addr_2->ai_addr, addr_2->ai_addrlen); 
	  if (rc < 0) {
	  	if (errno == EINPROGRESS) printf("EINPROGRESS in connect() - selecting\n"); 
			else printf("nie ma takiego hosta\n"); //syserr("connect");	  	
	  } else {
	  	printf("połączono!\n");
	  }


	  printf("tcp śpi...\n");
	  sleep(1);
//	  close(sock);
//	}

  return 0;
}




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
