void connect_w_to(void) { 
  int res, valopt; 
  struct sockaddr_in addr; 
  long arg; 
  fd_set myset; 
  struct timeval tv; 
  socklen_t lon; 

  // Create socket 
  soc = socket(AF_INET, SOCK_STREAM, 0); 

  // Set non-blocking 
  arg = fcntl(soc, F_GETFL, NULL); 
  arg |= O_NONBLOCK; 
  fcntl(soc, F_SETFL, arg); 

  // Trying to connect with timeout 
  addr.sin_family = AF_INET; 
  addr.sin_port = htons(2000); 
  addr.sin_addr.s_addr = inet_addr("192.168.0.1"); 
  res = connect(soc, (struct sockaddr *)&addr, sizeof(addr)); 

  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        tv.tv_sec = 15; 
        tv.tv_usec = 0; 
        FD_ZERO(&myset); 
        FD_SET(soc, &myset); 
        if (select(soc+1, NULL, &myset, NULL, &tv) > 0) { 
           lon = sizeof(int); 
           getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon); 
           if (valopt) { 
              fprintf(stderr, "Error in connection() %d - %s\n", valopt, strerror(valopt)); 
              exit(0); 
           } 
        } 
        else { 
           fprintf(stderr, "Timeout or error() %d - %s\n", valopt, strerror(valopt)); 
           exit(0); 
        } 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        exit(0); 
     } 
  } 
  // Set to blocking mode again... 
  arg = fcntl(soc, F_GETFL, NULL); 
  arg &= (~O_NONBLOCK); 
  fcntl(soc, F_SETFL, arg); 
  // I hope that is all 
}