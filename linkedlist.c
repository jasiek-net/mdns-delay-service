#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct sockaddr_in stack_data; //stack data type - set for integers, modifiable
typedef struct stack Node; //short name for the stack type
struct stack //stack structure format
{
    stack_data data;
    Node *next;
};
 
int stack_len(Node *node_head); //stack length
void stack_push(Node **node_head, stack_data d); //pushes a value d onto the stack
stack_data stack_pop(Node **node_head); //removes the head from the stack & returns its value
void stack_print(Node **node_head); //prints all the stack data
void stack_clear(Node **node_head); //clears the stack of all elements
void stack_snoc(Node **node_head, stack_data d); //appends a node
int stack_elem(Node **node_head, stack_data d); //checks for an element
 
// int main(void)
// {
//     Node *node_head = NULL; //pointer to stack head
 
//     //example usage:
//     stack_push(&node_head, 7); //push value 7 onto stack
//     printf("%d\n", node_head -> data); //show stack head value
//     stack_push(&node_head, 21); //push value 21 onto stack
//     stack_print(&node_head); //print the stack
//     if(stack_elem(&node_head, 7)) puts("found 7"); //does 7 belong to the stack?
//     stack_snoc(&node_head, 0); //append 0 to the end of the stack
//     stack_print(&node_head); //print the stack
//     printf("%d\n", stack_pop(&node_head)); //pop the stack's head
//     stack_print(&node_head); //print the stack
//     stack_clear(&node_head); //clear the stack
//     stack_print(&node_head); //print the stack
     
//     getchar();
//     return 0;
// }
 
int stack_len(Node *node_head)
{
    Node *curr = node_head;
    int len = 0;
     
    while(curr)
    {
        ++len;
        curr = curr -> next;
    }
    return len;
}
 
void stack_push(Node **node_head, stack_data d)
{
    Node *node_new = malloc(sizeof(Node));
     
    node_new -> data = d;
    node_new -> next = *node_head;
    *node_head = node_new;
}
 
stack_data stack_pop(Node **node_head)
{
    Node *node_togo = *node_head;
    stack_data d;
     
    if(node_head)
    {
        d = node_togo -> data;
        *node_head = node_togo -> next;
        free(node_togo);
    }
    return d;
}
 
void stack_print(Node **node_head) {
    Node *node_curr = *node_head;
     
    if(!node_curr)
        puts("the stack is empty");
    else {
        while(node_curr) {
            (void) printf("%s", inet_ntoa(node_curr->data.sin_addr));
            (void) printf(":");
            (void) printf("%d -> ", ntohs(node_curr->data.sin_port));
            node_curr = node_curr -> next;
        }
        putchar('\n');
    }
}
 
void stack_clear(Node **node_head)
{
    while(*node_head)
        stack_pop(node_head);
}
 
void stack_snoc(Node **node_head, stack_data d)
{
    Node *node_curr = *node_head;
     
    if(!node_curr)
        stack_push(node_head, d);
    else
    {
        //find the last node
        while(node_curr -> next)
            node_curr = node_curr -> next;
        //build the node after it
        stack_push(&(node_curr -> next), d);
    }
}
 
// int stack_elem(Node **node_head, stack_data d)
// {
//     Node *node_curr = *node_head;
     
//     while(node_curr)
//     {
//         if(node_curr -> data == d) //set for numbers, modifiable
//             return 1;
//         else
//             node_curr = node_curr -> next;
//     }
//     return 0;
// }