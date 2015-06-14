#define T_A 1 //Ipv4 address
#define T_PTR 12 /* domain name pointer */
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

void set_my_ip();
void set_my_host();

struct RES_RECORD *get_answer(char *buf);
struct QUERY *get_question(char *buf);

int create_answer(unsigned char *query, int type, unsigned char *buf, ssize_t *len);
int create_question(unsigned char *name, unsigned char *rdata, int type, unsigned char *buf, ssize_t *len);

void parse_msg(char * buf);
u_char* ReadName(unsigned char* reader, unsigned char* buffer, int* count);


