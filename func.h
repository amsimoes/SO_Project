/*
 * @author: António Miguel Egas Simões, Nº 2014198322
 */

#ifndef FUNC_HEADER
#define FUNC_HEADER

#include "main.h"

int verifica_ficheiros(char *fname);
void linha(char* nome);
int linhas(char *fname);
void convertName2RFC (unsigned char*,unsigned char*);
unsigned char* convertRFC2Name (unsigned char*,unsigned char*,int*);
void sendReply(unsigned short, unsigned char*, int, int, struct sockaddr_in);

#endif