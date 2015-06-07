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
struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};
 
//Structure of a Query
typedef struct
{
    unsigned char *name;
    struct QUESTION *ques;
} QUERY;

#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wunused-variable"

void parse_msg(char * buf);
void ansitonetwork(unsigned char* host, unsigned char* netwrk);
void get_record(unsigned char *query, int query_type, unsigned char *buf, ssize_t *len);
unsigned char *get_query(unsigned char* buf);

// Na przykład:
// na zapytanie o PTR dla _opoznienia._udp.local. odpowiadamy, że moj-komputer._opoznienia._udp.local.,
// otrzymawszy taką odpowiedź pytamy o A dla moj-komputer._opoznienia._udp.local.,
// na zapytanie o A dla moj-komputer._opoznienia._udp.local. odpowiadamy, że 192.168.1.42.

void *m_dns_receive(void *arg) {
  int sock = *(int *) arg;
  struct sockaddr addr;
  socklen_t addr_len = sizeof(struct sockaddr);
  int flags = 0;

  socklen_t snd_len;
  ssize_t len;
  unsigned char *query;
  unsigned char buf[MAX_BUF_SIZE];
  while(1) {
    len = recvfrom(sock, &buf, sizeof(buf), flags, &addr, &addr_len);
    if (len < 0) syserr("error on datagram from client socket");
    else {

      query = get_query(buf);
      parse_msg(buf);
      printf("size from recv: %d\n", (int) len);
      printf("query from msg: %s\n", query);
      // len = sizeof(buf);
      // snd_len = sendto(sock, buf, len, flags, &addr, addr_len);
      // if (snd_len != len) syserr("error on sending datagram to client socket");


      // na końcu zdezalokować query!!!
    }
  }
  (void) printf("finished exchange\n");
  return 0;
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
  get_record(query, T_PTR, buf, &len);
  printf("size: %d\n", len);

 // parse_msg(msg);
  // while(1) {
    snd_len = sendto(sock, buf, len, flags, &mdns_addr, addr_len);
    if (snd_len != len) syserr("sendto");
    printf("wysłano PTR\n");
    sleep(sec);    
  // }

  if (close(sock) == -1) { //very rare errors can occur here, but then
    syserr("close"); //it's healthy to do the check
  };
  return NULL;
}

u_char* ReadName(unsigned char* reader, unsigned char* buffer, int* count);

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
    name[i-1] = '\0'; //remove the last dot
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

void get_record(unsigned char *query, int query_type, unsigned char *buf, ssize_t *len) {
    printf("Resolving %s\n" , query);

    unsigned char *qname;
    struct DNS_HEADER *dns = NULL;
    struct QUESTION *qinfo = NULL;
  
    //Set the DNS structure to standard queries
    dns = (struct DNS_HEADER *) buf;
 
    dns->id = (unsigned short) htons(getpid());
    dns->qr = 0; //This is a query
    dns->opcode = 0; //This is a standard query
    dns->aa = 0; //Not Authoritative
    dns->tc = 0; //This message is not truncated
    dns->rd = 0; //Recursion Desired
    dns->ra = 0; //Recursion not available! hey we dont have it (lol)
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); //we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;
 
    //point to the query portion
    qname = (unsigned char*) &buf[sizeof(struct DNS_HEADER)];

    ansitonetwork(query, qname);

    qinfo = (struct QUESTION*) &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it

    qinfo->qtype = htons( query_type ); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(1);           //its internet (lol)

    *len = sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION);    
 }

