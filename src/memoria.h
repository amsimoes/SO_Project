/*
 * @author: António Miguel Egas Simões, Nº 2014198322
 */

#ifndef MEM_HEADER
#define MEM_HEADER

#include <sys/ipc.h>
#include <sys/shm.h> 
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "main.h"
#include "func.h"

#define MAX 256

// STRUCT
typedef struct config {
	int N_threads;
	char localDomain[MAX];
	char Named_pipe[MAX];
	char data[MAX];
	int n_domains;
	int flag; // impar -> modo manuntençao || par -> modo normal
	int saida; // 1 -> acabado de sair, ler para memoria
	char domains[][MAX];
}Config;

Config* ptr_config;

void mapear(char* ficheiro);
void file2memory(char *fname); 

#endif