/*
 * @author: António Miguel Egas Simões, Nº 2014198322
 */

#include "func.h"

// Retorna 1 se o Ficheiro existir
int verifica_ficheiros(char *fname) {
	if (access(fname, F_OK) == -1) {
		return 0;
	} else {
        return 1;
    }
}

// Retorna 1 se a lista estiver vazia 
int fila_vazia(Fila fila){
    int rc = pthread_mutex_unlock(&mutex);
    return (fila->next == NULL ? 1 : 0);
    rc = pthread_mutex_lock(&mutex);
}

void linha(char* nome){
    if(nome[strlen(nome)-1] == '\n'){
        nome[strlen(nome)-1] = '\0';
    }
}

int linhas(char *fname) {
	FILE* fp = fopen(fname,"r");
	int ch, lines;

    while(!feof(fp)) {
        ch = fgetc(fp);
        if(ch == '\n') {
            lines++;
        }
    }

    if(ch != '\n' && lines != 0) 
        lines++;

	fclose(fp);

	return lines;
}

/**
    sendReply: this method sends a DNS query reply to the client
    * id: DNS message id (required in the reply)
    * query: the requested query name (required in the reply)
    * ip_addr: the DNS lookup reply (the actual value to reply to the request)
    * sockfd: the socket to use for the reply
    * dest: the UDP package structure with the information of the DNS query requestor (includes it's IP and port to send the reply)
**/
void sendReply(unsigned short id, unsigned char* query, int ip_addr, int sockfd, struct sockaddr_in dest) {
        //printf("Inside sendReply\n");
        unsigned char bufReply[65536], *rname;
        char *rip;
        struct R_DATA *rinfo = NULL;
        
        //Set the DNS structure to reply (according to the RFC)
        struct DNS_HEADER *rdns = NULL;
        rdns = (struct DNS_HEADER *)&bufReply;
        rdns->id = id;
        rdns->qr = 1;
        rdns->opcode = 0;
        rdns->aa = 1;
        rdns->tc = 0;
        rdns->rd = 0;
        rdns->ra = 0;
        rdns->z = 0;
        rdns->ad = 0;
        rdns->cd = 0;
        rdns->rcode = 0;
        rdns->q_count = 0;
        rdns->ans_count = htons(1);
        rdns->auth_count = 0;
        rdns->add_count = 0;
        
        // Add the QUERY name (the same as the query received)
        rname = (unsigned char*)&bufReply[sizeof(struct DNS_HEADER)];
        convertName2RFC(rname , query);
        
        // Add the reply structure (according to the RFC)
        rinfo = (struct R_DATA*)&bufReply[sizeof(struct DNS_HEADER) + (strlen((const char*)rname)+1)];
        rinfo->type = htons(1);
        rinfo->_class = htons(1);
        rinfo->ttl = htonl(3600);
        rinfo->data_len = htons(sizeof(ip_addr)); // Size of the reply IP address

        // Add the reply IP address for the query name 
        rip = (char *)&bufReply[sizeof(struct DNS_HEADER) + (strlen((const char*)rname)+1) + sizeof(struct R_DATA)];
        memcpy(rip, (struct in_addr *) &ip_addr, sizeof(ip_addr));

        // Send DNS reply
        printf("\nSending Answer... ");
        if( sendto(sockfd, (char*)bufReply, sizeof(struct DNS_HEADER) + (strlen((const char*)rname) + 1) + sizeof(struct R_DATA) + sizeof(ip_addr),0,(struct sockaddr*)&dest,sizeof(dest)) < 0) {
            printf("FAILED!!\n");
        } else {
            printf("SENT!!!\n");
        }
}

/**
    convertRFC2Name: converts DNS RFC name to name
**/
u_char* convertRFC2Name(unsigned char* reader,unsigned char* buffer,int* count) {
    unsigned char *name;
    unsigned int p=0,jumped=0,offset;
    int i , j;
 
    *count = 1;
    name = (unsigned char*)malloc(256);
 
    name[0]='\0';
 
    while(*reader!=0) {
        if(*reader>=192) {
            offset = (*reader)*256 + *(reader+1) - 49152;
            reader = buffer + offset - 1;
            jumped = 1;
        } else {
            name[p++]=*reader;
        }
 
        reader = reader+1;
 
        if(jumped==0) {
            *count = *count + 1;
        }
    }
 
    name[p]='\0';
    if(jumped==1) {
        *count = *count + 1;
    }
 
    for(i=0;i<(int)strlen((const char*)name);i++) {
        p=name[i];
        for(j=0;j<(int)p;j++) {
            name[i]=name[i+1];
            i=i+1;
        }
        name[i]='.';
    }
    name[i-1]='\0';
    return name;
}

/**
    convertName2RFC: converts name to DNS RFC name
**/
void convertName2RFC(unsigned char* dns,unsigned char* host) {
    int lock = 0 , i;
    strcat((char*)host,".");
     
    for(i = 0 ; i < strlen((char*)host) ; i++) {
        if(host[i]=='.') {
            *dns++ = i-lock;
            for(;lock<i;lock++) {
                *dns++=host[lock];
            }
            lock++;
        }
    }
    *dns++='\0';
}