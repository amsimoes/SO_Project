/*
 * @author: António Miguel Egas Simões, Nº 2014198322
 */

#ifndef MAIN_HEADER
#define MAIN_HEADER

#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "memoria.h"
#include "func.h"

#define FREQ 30
#define N_DOMAINS 100
#define MAX 256

pthread_mutex_t mutex;
pthread_cond_t cond;

int shmID;

pid_t cfg_process;
pid_t stats_process;

struct stats {
    int total_pedidos;
    int pedidos_negados;
    int pedidos_local;
    int pedidos_externos;
    char data_hora_arranque[50];
    char data_info[50];
};

int t_p;
int p_n;
int p_l;
int p_e;
char d_a[50];
char u_i[50];


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
struct QUESTION
{
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
struct QUERY
{
    unsigned char *name;
    struct QUESTION *ques;
};

typedef struct fila *Fila;
typedef struct fila {
    char query[MAX];
    struct sockaddr_in origem;
    int dns_id;
    int socket;
    Fila next;    
}Dados;

typedef struct fila *fila_normal;
typedef struct fila *fila_prioridade;

typedef struct local *Local;
typedef struct local {
    char addr[MAX];
    char ip[MAX];
    Local next;
}Domains;

typedef struct local *l_domains;

struct thread_data {
    Local domains;
    Fila normal;
    Fila prioridade;
};

Fila cria_fila();
Local cria_lista();
void imprimir_fila(Fila f);
void insere_pedido(Fila f, Fila novo);
Fila receber_info(unsigned char* query, struct sockaddr_in* o, int dns_id, int socket);
void token_mmap(Local local, char* data);
void insere_domain(Local local, Local novo);
int fila_vazia(Fila fila);
void eliminar_pedido(Fila fila);

#endif