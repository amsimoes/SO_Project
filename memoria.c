/*
 * @author: António Miguel Egas Simões, Nº 2014198322
 */

#include "memoria.h"


void mapear(char* ficheiro) {
	verifica_ficheiros(ficheiro);

	int fd,i;
	struct stat sbuf;

	Config *ptr;
	if ((ptr = shmat(shmID, NULL, 0)) == (Config *) -1) {
		perror("error in shmat");
		exit(1);
	} 

	// Abertura e verificação do ficheiro
	if ((fd = open(ficheiro,O_RDONLY)) == -1) {
		perror("open");
		exit(1);
	}

	// Load do ficheiro para a estructura, que depois nos dá o size em bytes
	if (stat(ficheiro,&sbuf) == -1) {
		perror("stat");
		exit(1);
	}

	char* data = mmap((caddr_t)0, sbuf.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (data == (caddr_t)(-1)) {
		perror("mmap");
		exit(1);
	}

	char* temp = data;
	strcpy(ptr->data, temp);

	if (munmap(data, sbuf.st_size) == -1) {
		perror("Error un-mmaping the file.\n");
		exit(1);
	}

	close(fd);
}

void file2memory(char* fname) {
	int i;

	Config *sptr;
	if ((sptr = shmat(shmID, NULL, 0)) == (Config *) -1) {
		perror("error in shmat");
		exit(1);
	}

	sptr->n_domains = 0;

	FILE *f = fopen(fname, "r");

	fscanf(f, "Threads = %d\n",&sptr->N_threads);
	fscanf(f, "Domains = ");

	char tmp[MAX];

	fgets(tmp, MAX, f);
	linha(tmp);

	
	char* token = strtok(tmp, "; ");
	for(i=0; i < N_DOMAINS; i++) {
		if(token != NULL) {
			sptr->n_domains += 1;
			strcpy(sptr->domains[i], token);
			token = strtok(NULL, "; ");
		}
	}

	#ifdef DEBUG
	printf("Domains lidos:\n");
	for(i=0;i<sptr->n_domains;i++) {
		printf("%s\n", sptr->domains[i]);
	}
	#endif	

	fscanf(f, "LocalDomain = %s\n", sptr->localDomain);
	fscanf(f, "NamedPipeEstatisticas = %s", sptr->Named_pipe);

	fclose(f);
}
