#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
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
#include <net/if.h>

#include "linkedlist.h"
#include "opoznienia.h"
#include "err.h"
#include "mdns_extra.h"

// #pragma GCC diagnostic ignored "-Wpointer-sign"
// #pragma GCC diagnostic ignored "-Wunused-variable"

struct sockaddr_in my_addr;
char hostname[1024];
char host_opoznie[1024];
char host_ssh_tcp[1024];
char opoznie[] = "_opoznienia._udp.local.";
char ssh_tcp[] = "_ssh._tcp.local.";

void set_my_ip() {
  struct ifreq ifr;
  int fd;
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  ifr.ifr_addr.sa_family = AF_INET;
  // get ip from eth0 interface
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
  ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);  
  my_addr = *((struct sockaddr_in *)&ifr.ifr_addr);
}

void set_my_host() {
  char host[1024];
  host[1023] = '\0';
  gethostname(host, 1023);
  struct hostent* h;
  h = gethostbyname(host);
  strcpy(hostname, h->h_name);
  strcpy(host_opoznie, h->h_name);
  strcpy(host_ssh_tcp, h->h_name);
  strcat(host_opoznie, ".");
  strcat(host_ssh_tcp, ".");
  strcat(host_opoznie, opoznie);
  strcat(host_ssh_tcp, ssh_tcp);
  // printf("%s\n", hostname);
  // printf("%s\n", host_opoznie);
  // printf("%s\n", host_ssh_tcp);
}

// ansi to network
void aton(unsigned char *host, unsigned char *netwrk) {
    int lock = 0 , i;
    for(i = 0 ; i < strlen((char*)host) ; i++) {
        if(host[i] == '.') {
            *netwrk ++= i-lock;
            for(; lock < i; lock++) {
                *netwrk ++= host[lock];
            }
            lock++;
        }
    }
    *netwrk ++= '\0';
}

void create_mdns_header(unsigned char *buf, int type) {
  struct DNS_HEADER *dns = NULL;

  //Set the DNS structure to standard queries
  dns = (struct DNS_HEADER *) buf;

  dns->id = (unsigned short) htons(0);
  dns->opcode = 0; //This is a standard query
  dns->aa = 0; //Not Authoritative
  dns->tc = 0; //This message is not truncated
  dns->rd = 0; //Recursion Desired
  dns->ra = 0; //Recursion not available! hey we dont have it (lol)
  dns->z = 0;
  dns->ad = 0;
  dns->cd = 0;
  dns->rcode = 0;
  dns->q_count = 0;
  dns->ans_count = 0;
  dns->auth_count = 0;
  dns->add_count = 0;

  if (type == 0) {  // type: question (0) or answer (1)
    dns->qr = 0; //This is a query
    dns->q_count = htons(1); //we have only 1 question
  } else
  if (type == 1) {
    dns->qr = 1; //This is an answer
    dns->ans_count = htons(1);
  }
}

int create_answer(unsigned char *query, int type, unsigned char *buf, ssize_t *len) {
  unsigned char *name, *rdata;
  struct R_DATA *resource = NULL;
  create_mdns_header(buf, 1);
  name = (unsigned char *) &buf[sizeof(struct DNS_HEADER)];
  aton(query, name);
  resource = (struct R_DATA*) &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)name) + 1)]; //fill it
  resource->type = htons( type ); //type of the query, A or PTR
  resource->_class = htons(1); //its internet
  resource->ttl = 0; // TTL in seconds
  rdata = (unsigned char *) &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)name) + 1) + sizeof(struct R_DATA)];
  *len = sizeof(struct DNS_HEADER) + sizeof(struct R_DATA) + (strlen((const char*)name) + 1);

  if (type == T_PTR) {
    if (!strcmp((const char *) query, opoznie))
      aton((unsigned char *)host_opoznie, rdata);
    else if (!strcmp((const char *) query, ssh_tcp))
      aton((unsigned char *)host_ssh_tcp, rdata);
    else
      return 0;
    
    resource->data_len = htons( strlen((const char*)rdata) + 1 ); // the lenght of RR specific data in octets
    *len += (strlen((const char*)rdata)+1);
    return 1;

  } else if (type == T_A) {
    // printf("send ip: %lu\n", my_addr.sin_addr);
    memcpy(rdata, &my_addr.sin_addr, 4);  // copy my IP addr
    resource->data_len = htons(4); // size of IP addr
    *len += 4;
    return 1;
  } else
    return 0;
}

int create_question(unsigned char *name, unsigned char *rdata, int type, unsigned char *buf, ssize_t *len) {
  if (type == T_A && (strstr((const char *)name, opoznie) || strstr((const char *)name, ssh_tcp))) {
      uint32_t *p;
      p = (uint32_t *) rdata;
      // struct sockaddr_in a;
      // a.sin_addr.s_addr = (*p);
      // printf("create_question: ip %lu\n", *p);
      // printf("RCV A name: %s, IPv4 address : %s\n", name, inet_ntoa(a.sin_addr));            

    if (strstr((const char *)name, opoznie)) {
      create_or_add(*p, "udp");
    } else if (strstr((const char *)name, ssh_tcp)) {
      create_or_add(*p, "tcp");
    }
    return 0;
  } else if (type == T_PTR && (!strcmp((const char *) name, opoznie) || !strcmp((const char *) name, ssh_tcp))) {
      create_mdns_header(buf, 0);
      unsigned char *qname;
      qname = (unsigned char*) &buf[sizeof(struct DNS_HEADER)];
      aton(rdata, qname);
      struct QUESTION *qinfo = NULL;
      qinfo = (struct QUESTION*) &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
      qinfo->qtype = htons( T_A ); // IP question
      qinfo->qclass = htons(1);
      *len = sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION);
      return 1;
    } else if (type == 0) { // first, multicast question
      unsigned char *qname;
      create_mdns_header(buf, 0);
      qname = (unsigned char*) &buf[sizeof(struct DNS_HEADER)];
      aton(name, qname);
      struct QUESTION *qinfo = NULL;
      qinfo = (struct QUESTION*) &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
      qinfo->qtype = htons( T_PTR );
      qinfo->qclass = htons(1); // 1 for Internet
      *len = sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION);
    }
    return 0;
}

struct QUERY *get_question(unsigned char *buf) {
    struct QUERY *questions = malloc(20 * sizeof(struct QUERY));
    struct DNS_HEADER *dns = NULL;
    unsigned char *reader;
    int i, stop;

    dns = (struct DNS_HEADER*) buf;
    reader = &buf[sizeof(struct DNS_HEADER)];
 
    //Start reading QUESTIONS
    stop = 0;
    for(i = 0; i < ntohs(dns->q_count); i++) {
        questions[i].name = ReadName(reader, buf, &stop);
        reader = reader + stop;
        questions[i].ques = (struct QUESTION*)(reader);
        reader = reader + sizeof(struct QUESTION);
    }

  return questions;
}


struct RES_RECORD *get_answer(unsigned char *buf) {
    struct RES_RECORD *answers = malloc(20 * sizeof(struct RES_RECORD));
    struct DNS_HEADER *dns = NULL;
    unsigned char *reader;
    int i, j, stop;

    dns = (struct DNS_HEADER*) buf;
    reader = &buf[sizeof(struct DNS_HEADER)];
    //Start reading ANSWERS
    stop = 0;
    for(i = 0; i < ntohs(dns->ans_count); i++) {
        answers[i].name = ReadName(reader, buf, &stop);
        reader = reader + stop;

        answers[i].resource = (struct R_DATA*) (reader);
        reader = reader + sizeof(struct R_DATA);

        // tu trzeba będzie zmienić też na else T_PTR
        if(ntohs(answers[i].resource->type) == T_A) { //if its an ipv4 address
            answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));
 
            for(j = 0 ; j < ntohs(answers[i].resource->data_len); j++)
                answers[i].rdata[j] = reader[j];

            answers[i].rdata[ ntohs(answers[i].resource->data_len) ] = '\0';
            reader = reader + ntohs(answers[i].resource->data_len);
        } else {
            answers[i].rdata = ReadName(reader, buf, &stop);
            reader = reader + stop;
        }
    }
    return answers;
}

u_char* ReadName(unsigned char* reader, unsigned char* buffer, int* count) {
    unsigned char *name;
    unsigned int p = 0, jumped = 0,offset;
    int i, j;
 
    *count = 1;
    name = (unsigned char*)malloc(256);
 
    name[0]='\0';
 
    //read the names in 3www6google3com format
    while(*reader != 0) {
        if(*reader>=192) {
            offset = (*reader)*256 + *(reader+1) - 49152; //49152 = 11000000 00000000 ;)
            reader = buffer + offset - 1;
            jumped = 1; //we have jumped to another location so counting wont go up!
        } else
            name[p++]=*reader;
 
        reader = reader+1;
 
        if(jumped==0)
            *count = *count + 1; //if we havent jumped to another location then we can count up
    }
 
    name[p]='\0'; //string complete
    if(jumped==1)
        *count = *count + 1; //number of steps we actually moved forward in the packet
 
    //now convert 3www6google3com0 to www.google.com
    for(i=0;i<(int)strlen((const char*)name);i++) {
        p=name[i];
        for(j = 0; j < (int)p; j++) {
            name[i] = name[i+1];
            i = i+1;
        }
        name[i] = '.';
    }
    name[i] = '\0'; //remove the last dot
    return name;
}