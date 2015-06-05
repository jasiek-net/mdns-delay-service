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
 
void stack_push(Node **node_head, struct sockaddr_in addr) {
    Node *node_new = malloc(sizeof(Node));
    node_new -> host.addr = addr;
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
            (void) printf("%s", inet_ntoa(ptr->host.addr.sin_addr));
            (void) printf(":");
            (void) printf("%d -> ", ntohs(ptr->host.addr.sin_port));
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

stack_data *stack_elem(Node **node_head, struct sockaddr *sa)
{
    Node *ptr = *node_head;
     
    while(ptr) {
        if(strcmp(sa->sa_data,  sa->sa_data)) //set for numbers, modifiable
            ptr = ptr->next;
        else return &ptr->host;
    }
    return NULL;
}