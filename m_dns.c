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

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>


#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 // canonical name
#define T_SOA 6 /* start of authority zone */
#define T_PTR 12 /* domain name pointer */
#define T_MX 15 //Mail server
#define MAX_BUF_SIZE 65536

//DNS header structure
struct DNS_HEADER
{
    unsigned short id; // identification number
 
    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag
 
    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available
 
    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};
 
//Constant sized fields of query structure
struct QUESTION {
    unsigned short qtype;
    unsigned short qclass;
};
 
//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)
 
//Pointers to resource record contents
struct RES_RECORD {
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

//Structure of a Query
struct QUERY {
    unsigned char *name;
    struct QUESTION *ques;
};

#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wunused-variable"

struct RES_RECORD *get_answer(char *buf);
struct QUERY *get_question(char *buf);

void parse_msg(char * buf);
void ansitonetwork(unsigned char* host, unsigned char* netwrk);
void get_record(unsigned char *query, int query_type, int qora, unsigned char *buf, ssize_t *len);
unsigned char *get_query(unsigned char* buf);

u_char* ReadName(unsigned char* reader, unsigned char* buffer, int* count);


// Na przykład:
// na zapytanie o PTR dla _opoznienia._udp.local. odpowiadamy, że moj-komputer._opoznienia._udp.local.,
// otrzymawszy taką odpowiedź pytamy o A dla moj-komputer._opoznienia._udp.local.,
// na zapytanie o A dla moj-komputer._opoznienia._udp.local. odpowiadamy, że 192.168.1.42.

void get_record(unsigned char *query, int query_type, int qora, unsigned char *buf, ssize_t *len) {
//    printf("Resolving %s\n" , query);

    unsigned char *qname;
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

  if (qora == 0) {  // qora - question (0) or answer (1)
    dns->qr = 0; //This is a query
    dns->q_count = htons(1); //we have only 1 question

    qname = (unsigned char*) &buf[sizeof(struct DNS_HEADER)];
    ansitonetwork(query, qname);

    struct QUESTION *qinfo = NULL;
    qinfo = (struct QUESTION*) &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
    qinfo->qtype = htons( query_type ); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(1);           //its internet (lol)
    *len = sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION);    

  } else {
    dns->qr = 1; //This is an answer
    dns->ans_count = htons(1);

    qname = (unsigned char*) &buf[sizeof(struct DNS_HEADER)];
    ansitonetwork(query, qname);

    struct R_DATA *qinfo = NULL;
    qinfo = (struct R_DATA*) &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
    qinfo->type = htons( query_type ); //type of the query , A , MX , CNAME , NS etc
    qinfo->_class = htons(1);           //its internet (lol)
    qinfo->ttl = 0; // TTL in seconds
    qinfo->data_len = 0; // the lenght of RR specific data in octets,
    // rdata: 
    // Each (or rather most) resource record types have a specific RDATA format which reflect their resource record format as defined below: 
    // A: IP Address:  Unsigned 32-bit value representing the IP address
    // 
    // PTR: Name:  The host name that represents the supplied IP address. May be a label, pointer or any combination.

    *len = sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION);
  }
 }


char *get_hostname() {
  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
  struct hostent* h;
  h = gethostbyname(hostname);
  return h->h_name;
}

char *get_hostip() {
  int fd;
  struct ifreq ifr;
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  ifr.ifr_addr.sa_family = AF_INET;
  // get ip from eth0 interface
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
  ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);
  return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

void *m_dns_receive(void *arg) {
  int sock = *(int *) arg;
  struct sockaddr addr;
  socklen_t addr_len = sizeof(struct sockaddr);
  int flags = 0, i;

  socklen_t snd_len;
  ssize_t len;
  unsigned char *query;
  unsigned char buf[MAX_BUF_SIZE];
  struct DNS_HEADER *dns = NULL;

  char *hostname = strcat(get_hostname(), "._opoznienia._udp.local.");
  char *hostip = strcat(get_hostip(), ".");
//  printf("HOST: %s, %s\n", hostname, hostip);

  int add_new = 0;
  while(1) {
    len = recvfrom(sock, &buf, sizeof(buf), flags, &addr, &addr_len);
    if (len < 0) syserr("error on datagram from client socket");
    else {
      dns = (struct DNS_HEADER*) buf;

      if (ntohs(dns->q_count)) {  // we've got question over here!
        struct QUERY *questions = get_question(buf);

        for(i=0 ; i < ntohs(dns->q_count) ; i++) {
          
          if (ntohs(questions[i].ques->qtype) == T_PTR && strcmp(questions[i].name, "_opoznienia._udp.local.") == 0) {
            printf("2 A T_PTR: %s\n", hostname);
            get_record(hostname, T_PTR, 1, buf, &len); // answer with hostname._opoznienia._udp.local.
          }
          else if (ntohs(questions[i].ques->qtype) == T_A && strcmp(questions[i].name, hostname) == 0) {
            printf("4 A T_A: %s\n", hostip);
            get_record(hostip, T_A, 1, buf, &len);    // answer with host ip
          }
        }

      } else if (ntohs(dns->ans_count)) { // we've got answer over here!
        struct RES_RECORD *answers = get_answer(buf);

        for(i=0 ; i < ntohs(dns->ans_count) ; i++) {

          if(ntohs(answers[i].resource->type) == T_PTR && strcmp(answers[i].name, hostname) == 0) {
            printf("3 Q T_A: %s\n", hostname);
            get_record(hostname, T_A, 0, buf, &len);
          }
          else if (ntohs(answers[i].resource->type) == T_A) {
            printf("5 Add  : %s\n", answers[i].name);
            add_new = 1;
          }
        }
      }
      if (!add_new) {
        snd_len = sendto(sock, buf, len, flags, &addr, addr_len);
        if (snd_len != len) syserr("error on sending datagram to client socket");        
      } else
        add_new = 0;

      // na końcu zdezalokować query!!!
    }
  }
  (void) printf("finished exchange\n");
  return 0;

        // struct sockaddr_in a;
        // printf("\nAnswer Records : %d \n" , ntohs(dns->ans_count) );
        // for(i=0 ; i < ntohs(dns->ans_count) ; i++) {
        //     printf("Name : %s \n", answers[i].name);
        //     printf("  type : %d \n", ntohs(answers[i].resource->type));
        //     printf("  _class : %d \n", ntohs(answers[i].resource->_class));
        //     printf("  _ttl : %d \n", ntohs(answers[i].resource->ttl));
        //     printf("  data_len : %d \n", ntohs(answers[i].resource->data_len));

        //     if( ntohs(answers[i].resource->type) == T_A) { //IPv4 address
        //         long *p;
        //         p = (long*)answers[i].rdata;
        //         a.sin_addr.s_addr=(*p); //working without ntohl
        //         printf("  has IPv4 address : %s\n",inet_ntoa(a.sin_addr));
        //     }
             

}


struct QUERY *get_question(char *buf) {
    struct QUERY questions[20];
    struct DNS_HEADER *dns = NULL;
    unsigned char *reader;
    int i , j , stop;

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


struct RES_RECORD *get_answer(char *buf) {
    struct RES_RECORD answers[20];
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

void *m_dns(void *arg) {
  int sec = *(int *) arg;
  int sock, flags = 0;
  struct sockaddr mdns_addr;
  socklen_t addr_len = sizeof(struct sockaddr);

  unsigned char buf[MAX_BUF_SIZE];
  unsigned char query[] = "_opoznienia._udp.local.";
  
  sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
  if (sock < 0) syserr("socket");
  int one = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  ((struct sockaddr_in *) &mdns_addr)->sin_family = AF_INET;
  ((struct sockaddr_in *) &mdns_addr)->sin_addr.s_addr = inet_addr("127.0.0.1"); // address IP change to 224.0.0.251
  ((struct sockaddr_in *) &mdns_addr)->sin_port = htons(10001);  // change to 5353

  if (bind(sock, &mdns_addr, addr_len) < 0) syserr("bind");

  pthread_t m_dns_receive_t;
  if (pthread_create(&m_dns_receive_t, 0, m_dns_receive, &sock) != 0) syserr("pthread_create");

  ssize_t snd_len, len;
  get_record(query, T_PTR, 0, buf, &len); // 0 means query
  printf("size: %d\n", len);

 // parse_msg(msg);
  while(1) {
    snd_len = sendto(sock, buf, len, flags, &mdns_addr, addr_len);
    if (snd_len != len) syserr("sendto");
    printf("1 Q T_PTR: %s\n", query);
    sleep(sec);    
  }

  if (close(sock) == -1) { //very rare errors can occur here, but then
    syserr("close"); //it's healthy to do the check
  };
  return NULL;
}


void parse_msg(char * buf) {
    // but why?
    struct sockaddr_in a;

    unsigned char *qname, *reader;
    int i , j , stop;
  
    struct DNS_HEADER *dns = NULL;

    dns = (struct DNS_HEADER*) buf;
    qname = (unsigned char*) &buf[sizeof(struct DNS_HEADER)];

    //move ahead of the dns header and the query field
    reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*) qname) + 1) + sizeof(struct QUESTION)];
 
    printf("\nThe response contains : ");
    printf("\n %d Questions.", ntohs(dns->q_count));
    printf("\n %d Answers.", ntohs(dns->ans_count));
    printf("\n %d Authoritative Servers.", ntohs(dns->auth_count));
    printf("\n %d Additional records.\n\n", ntohs(dns->add_count));
 
    struct RES_RECORD answers[20], auth[20], addit[20]; //the replies from the DNS server

    //Start reading ANSWERS
    stop = 0;
    for(i = 0; i < ntohs(dns->ans_count); i++) {
        answers[i].name = ReadName(reader, buf, &stop);
        reader = reader + stop;

        answers[i].resource = (struct R_DATA*)(reader);
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
 
    //read authorities
    for(i = 0; i < ntohs(dns->auth_count); i++) {
        auth[i].name = ReadName(reader, buf, &stop);
        reader += stop;
 
        auth[i].resource = (struct R_DATA*)(reader);
        reader += sizeof(struct R_DATA);
 
        auth[i].rdata = ReadName(reader, buf, &stop);
        reader+=stop;
    }
 
    //read additional
    for(i=0; i < ntohs(dns->add_count); i++) {
        addit[i].name = ReadName(reader, buf, &stop);
        reader += stop;
 
        addit[i].resource = (struct R_DATA*)(reader);
        reader += sizeof(struct R_DATA);
 
        if(ntohs(addit[i].resource->type) == T_A) {
            addit[i].rdata = (unsigned char*)malloc(ntohs(addit[i].resource->data_len));
            for(j = 0; j < ntohs(addit[i].resource->data_len); j++)
              addit[i].rdata[j] = reader[j];
 
            addit[i].rdata[ntohs(addit[i].resource->data_len)]='\0';
            reader += ntohs(addit[i].resource->data_len);
        } else {
            addit[i].rdata = ReadName(reader, buf, &stop);
            reader += stop;
        }
    }
 
    //print answers
    printf("\nAnswer Records : %d \n" , ntohs(dns->ans_count) );
    for(i=0 ; i < ntohs(dns->ans_count) ; i++) {
        printf("Name : %s ", answers[i].name);
 
        if( ntohs(answers[i].resource->type) == T_A) { //IPv4 address
            long *p;
            p = (long*)answers[i].rdata;
            a.sin_addr.s_addr=(*p); //working without ntohl
            printf("has IPv4 address : %s",inet_ntoa(a.sin_addr));
        }
         
        if(ntohs(answers[i].resource->type) == 5) //Canonical name for an alias
            printf("has alias name : %s",answers[i].rdata);
 
        printf("\n");
    }
 
    //print authorities
    printf("\nAuthoritive Records : %d \n" , ntohs(dns->auth_count) );
    for( i=0 ; i < ntohs(dns->auth_count) ; i++) {
        printf("Name : %s ",auth[i].name);
        if(ntohs(auth[i].resource->type)==2)
            printf("has nameserver : %s",auth[i].rdata);
        printf("\n");
    }
 
    //print additional resource records
    printf("\nAdditional Records : %d \n" , ntohs(dns->add_count) );
    for(i=0; i < ntohs(dns->add_count) ; i++) {
        printf("Name : %s ",addit[i].name);
        if(ntohs(addit[i].resource->type) == 1) {
            long *p;
            p=(long*)addit[i].rdata;
            a.sin_addr.s_addr=(*p);
            printf("has IPv4 address : %s",inet_ntoa(a.sin_addr));
        }
        printf("\n");
    }
    return;
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

void ansitonetwork(unsigned char* host, unsigned char* netwrk) {
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

unsigned char * get_query(unsigned char* buf) {
  unsigned char *query;
  unsigned char *result;
  query = &buf[sizeof(struct DNS_HEADER)];
  result = malloc(strlen((const char *) query));
//  printf("string len: %d\n", (int) strlen((const char *) query));
  unsigned int p;
  int i, j;
  for (i = 0; i < (int) strlen((const char *)query); i++) {
    p = query[i];
    for(j = 0; j < (int)p; j++) {
      result[i] = query[i + 1];
      i++;
    }
    result[i] = '.';
  }
  result[i] = '\0';
  return result;
  // printf("result len: %d\n", (int) strlen((const char *) result));
  // printf("Odpowiedź: %s\n", result);
}

