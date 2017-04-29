/*
 * @author: António Miguel Egas Simões, Nº 2014198322
 */

#include "main.h"


int valido = 0;
int n_linhas;

pthread_t *p_threads;

void init_shm() {
	#ifdef DEBUG
	printf("Criacao e Mapeamento da Memoria Partilhada.\n");
	#endif
	if((shmID = shmget(IPC_PRIVATE, sizeof(Config), IPC_CREAT | 0600)) == -1) {
		perror("error in shmget");
		exit(1);
	}

	if ((ptr_config = shmat(shmID, NULL, 0)) == (Config *) -1) {
		perror("error in shmat");
		exit(1);
	}
}

void stats_print() {
	printf("\n*** [ STATS PRINTER ] ***\n");
	printf("Data/Hora de arranque do Servidor = %s\n", d_a);
	printf("Total Pedidos = %d\n", t_p);
	printf("Pedidos Negados = %d\n", p_n);
	printf("Pedidos Locais = %d\n", p_l);
	printf("Pedidos Externos = %d\n", p_e);
	printf("Data/Hora da ultima informacao obtida = %s\n", u_i);
	printf("ID do Processo de Config = %d\n", cfg_process);
	printf("\n-- Waiting for DNS message --\n");
}

void handler_alarm(int sig) {
	stats_print();
	signal(SIGALRM, handler_alarm);
	alarm(FREQ);
}

void termina() {
	char* n_pipe = ptr_config->Named_pipe;
	unlink(n_pipe);

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);

	int i;

	for(i=0;i<ptr_config->N_threads;i++) {
		pthread_kill(p_threads[i], SIGINT);
	}

	while(wait(NULL) != -1);

	if(cfg_process == 0 && stats_process != 0) {
		printf("CFG a terminar...\n");
		exit(0);
	} 
	if(stats_process == 0 && cfg_process != 0) {
		printf("STATS a terminar...\n");
		exit(0);
	}

	if(shmID > 0) {
		shmdt(NULL);
		shmctl(shmID, IPC_RMID, NULL);	
	}

	exit(0);
}

void ctrlHandler() { // Handler para o SIGINT para encerrar o programa (CTRL+C)
	#ifdef DEBUG
	printf("Bye bye!\n");
	#endif
	termina();
}

void handler_CFG(int sig) {
	ptr_config->flag += 1;
	if(ptr_config->flag % 2 == 0) {
		printf("\nSignal SIGUSR1 detetado!\n** Saida do Modo de Manutencao **\n");
		ptr_config->saida = 1;
	} else {
		printf("Signal SIGUSR1 detetado\n** Entrada no Modo de Manutencao **\n");
	}
	signal(SIGUSR1, handler_CFG);
}

void* requester(void* thr_data) {
	signal(SIGINT, ctrlHandler);
	int rc;

	rc = pthread_mutex_lock(&mutex);
	#ifdef DEBUG
	printf("Thread %lu created!\n", pthread_self());
	#endif

	int found, i;

	char dig[30];
	char awk[30];

	FILE* file;

	char* ip;

	struct thread_data t_d;
	t_d = *((struct thread_data*)thr_data);

	Local head_local = t_d.domains;

	Fila head_normal = t_d.normal;
	Fila aux_normal;
	Fila head_prior = t_d.prioridade;
	Fila aux_prior;

	
	while(1) {
		rc = pthread_cond_wait(&cond, &mutex);
		#ifdef DEBUG
		printf("Thread working ID = %lu\n", pthread_self());
		#endif

		if(fila_vazia(head_prior) == 0) { // Lista local nao vazia
			#ifdef DEBUG
			imprimir_fila(t_d.prioridade);
			#endif

			#ifdef DEBUG
			printf("LOCAL thread...\n");
			#endif

			aux_prior = head_prior->next;
			Local local = head_local->next;

			#ifdef DEBUG
			printf("Dominio = %s\n", aux_prior->query);
			#endif

			i = 0;
			found = 0;
			while(i<n_linhas && local != NULL){
				if(strcmp(aux_prior->query, local->addr) == 0) {
					ip = local->ip;
					found = 1;
					break;
				}
				local = local->next;
				i++;
			}

			if(found != 0) {
				sendReply(aux_prior->dns_id, aux_prior->query, inet_addr(local->ip), aux_prior->socket, aux_prior->origem);
			} else {
				printf("IP de endereco local pedido nao encontrado!\n.");
				sendReply(aux_prior->dns_id, aux_prior->query, inet_addr("0.0.0.0"), aux_prior->socket, aux_prior->origem);
			}
			eliminar_pedido(head_prior);
			printf("\n\n-- Waiting for DNS message --\n\n");
		} else {	// Não há pedidos locais -> Tratar dos externos!
			#ifdef DEBUG
			imprimir_fila(head_normal);
			#endif

			aux_normal = head_normal->next;
			
			strcpy(dig, "dig +short ");
			
			strcpy(awk, " | awk \"{ print ; exit }\"");

			strcat(dig, aux_normal->query);
			strcat(dig, awk);

			printf("dig = %s\n", dig);

			char buffer[100];

			if((file = popen(dig, "r")) == NULL) {
				perror("Error on pipe or fork.\n");
				exit(1);
			}

			fgets(buffer, 100, file);
			pclose(file);

			#ifdef DEBUG
			printf("IP is = %s\n", buffer);
			#endif

			printf("\nQUERY A TRATAR = %d\n", aux_normal->dns_id);
			printf("QUERY HEAD = %d\n", head_normal->next->dns_id);

			if(buffer != NULL && buffer[0] == '\0') {
				printf("IP nao encontrado... A enviar 0.0.0.0...\n");
				sendReply(aux_normal->dns_id, aux_normal->query, inet_addr("0.0.0.0"), aux_normal->socket, aux_normal->origem);
			} else {
				sendReply(aux_normal->dns_id, aux_normal->query, inet_addr(buffer), aux_normal->socket, aux_normal->origem);
			}
			eliminar_pedido(head_normal);
			printf("\n\n-- Waiting for DNS message --\n\n");
		}
	}
	rc = pthread_mutex_unlock(&mutex);
}

int main( int argc , char *argv[]) {
	signal(SIGINT, ctrlHandler);


	n_linhas = linhas("localdns.txt");

	#ifdef DEBUG
	printf("Verificacao dos ficheiros presentes.\n");
	#endif

	if(verifica_ficheiros("localdns.txt") == 0) {
		printf("Ficheiro localdns.txt inexistente!\n");
		exit(1);
	}
	if(verifica_ficheiros("config.txt") == 0) {
		printf("Ficheiro config.txt inexistente!\n");
		exit(1);
	}

	#ifdef DEBUG
	printf("Inicializacoes.\n");
	#endif
	unsigned char buf[65536], *reader, *reader2, qname;
	int sockfd, stop;
	struct DNS_HEADER *dns = NULL;
	struct stats stat;

	struct sockaddr_in servaddr, dest;
	socklen_t len;

	int i, np, r;

	cfg_process = 1;
	stats_process = 1;

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	#ifdef DEBUG
	printf("Criacao das Filas Normal e Prioridade e da lista de Domains locais.\n");
	#endif
	Fila fila_normal = cria_fila();
	Fila fila_prioridade = cria_fila();

	Local l_domains = cria_lista();

	// Hora de arranque do Servidor
	time_t clk = time(NULL);
	strcpy(stat.data_hora_arranque, ctime(&clk));

	// Inicializaco Memoria Partilhada
	init_shm();
	
	ptr_config->flag = 0;

	printf("[PAI - MAIN] PID Main = %d PID DO MEU PAI = %d\n", getpid(), getppid());

	char* data;

	#ifdef DEBUG
	printf("Criacao processo de configuracoes.\n");
	#endif
	if ((cfg_process = fork()) == 0) {
		signal(SIGUSR1, handler_CFG);
		#ifdef DEBUG
		printf("RELEASE CFG.\n");
		printf("Mapeamento de localdns.txt.\n");
		#endif
		mapear("localdns.txt");
		#ifdef DEBUG
		printf("Leitura de config.txt para estrutura em memoria partilhada.\n");
		#endif
		file2memory("config.txt");
		if(signal(SIGALRM, SIG_IGN) == SIG_ERR) {
			perror("SIGALRM Ignore error");
			exit(1);
		}
	} else {
		usleep(200000);

		if(signal(SIGUSR1, SIG_IGN) == SIG_ERR) {
			perror("Signal ignore error");
			exit(1);
		}

		#ifdef DEBUG
		printf("RELEASE MAIN.\n");
		#endif

		#ifdef DEBUG
		printf("Extrair enderecos e ip's locais do mmap\n");
		#endif
		token_mmap(l_domains, ptr_config->data);

		// Check arguments
		if(argc <= 1) {
			printf("Usage: %s <port>\n", argv[0]);
			exit(1);
		}
		
		// Get server UDP port number
		int port = atoi(argv[1]);
		
		if(port <= 0) {
			printf("Usage: %s <port>\n", argv[0]);
			exit(1);
		}

		// ****************************************
		// Create socket & bind
		// ****************************************
		// Create UDP socket
	    sockfd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries
		if (sockfd < 0) {
   	      printf("ERROR opening socket.\n");
			 exit(1);
		}

		// Prepare UDP to bind port
		bzero(&servaddr,sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr=htonl(INADDR_ANY); 
		servaddr.sin_port=htons(port);
		
		// ****************************************
		// Receive questions
		// ****************************************

		// Bind application to UDP port
		int res = bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
		
		if(res < 0) {
	         printf("Error binding to port %d.\n", servaddr.sin_port);
			 
			 if(servaddr.sin_port <= 1024) {
				 printf("To use ports below 1024 you may need additional permitions. Try to use a port higher than 1024.\n");
			 } else {
				 printf("Please make sure this UDP port is not being used.\n");
			 }
			 exit(1);
		}


		#ifdef DEBUG
		printf("Criacao named pipe para as estatisticas\n.");
		#endif
		char* n_pipe = ptr_config->Named_pipe;
		if(verifica_ficheiros(n_pipe) == 1) {
			printf("Named pipe ja existente! A eliminar...\n");
			unlink(n_pipe);
		}
		if(((np = mkfifo(n_pipe, O_CREAT|O_EXCL|0600)<0)) && (errno != EEXIST)) {
			perror("mkfifo");
			exit(1);
		} 
		if (np != 0) {
			fprintf(stderr, "Impossivel criar fifo %s\n", n_pipe);
			return 1;
		}
		if((np = open(n_pipe, O_RDWR)) < 0) {
			perror("Opening Named Pipe");
			exit(1);
		}
	}

	#ifdef DEBUG
	printf("A sair = %d\n", getpid());
	#endif

	#ifdef DEBUG
	printf("Criacao Processo de estatisticas\n");
	#endif
	if(cfg_process != 0) {
		stats_process = fork();
		if(stats_process == 0) {
			if(signal(SIGUSR1, SIG_IGN) == SIG_ERR) {
				perror("Signal ignore error");
				exit(1);
			}
			#ifdef DEBUG
			printf("RELEASE STATS.\n");
			#endif
		} else {
			usleep(200000);
			if(signal(SIGALRM, SIG_IGN) == SIG_ERR) {
				perror("SIGALRM Ignore error");
				exit(1);
			}
		}
	}

	#ifdef DEBUG 
	printf("Quem chega primeiro = %d\n", getpid()); 
	#endif

	if(cfg_process != 0 && stats_process != 0) {
		#ifdef DEBUG
		printf("A preencher struct para argumento da funcao do thread.\n");
		#endif
		struct thread_data t_data;
		t_data.normal = fila_normal;
		t_data.prioridade = fila_prioridade;
		t_data.domains = l_domains;

		int N_THREADS = ptr_config->N_threads;

		p_threads = malloc(sizeof(*p_threads) * N_THREADS);

		#ifdef DEBUG
		printf("A criar a pool de threads.\n");
		#endif

		for(i=0;i < N_THREADS;i++) {
			pthread_create(p_threads+i, NULL, requester, (void*)&t_data);
		}
		usleep(50000);

		printf("\n\n-- Waiting for DNS message --\n\n");
	}

	while(1) {
		if((cfg_process == 0) && (ptr_config->saida == 1)) {	// MODO MANUNTENCAO
			file2memory("config.txt");
			ptr_config->saida = 0;
			usleep(100000);
			printf("\n-- Waiting for DNS message --\n");
		}
		if(cfg_process != 0 && stats_process != 0) {		// GESTOR PEDIDOS
			usleep(200000);
			
			len = sizeof(dest);

			// Thread blocking!
			if(recvfrom (sockfd, (unsigned char*)buf, 65536 , 0 , (struct sockaddr*)&dest , &len) < 0) {
				perror("Error in recvfrom");
				exit(1);
			}

			printf("DNS message received\n");

			// Process received message
			dns = (struct DNS_HEADER*) buf;
			qname = (unsigned char)buf[sizeof(struct DNS_HEADER)];
			reader = &buf[sizeof(struct DNS_HEADER)];

			// We only need to process the questions
			// We only process DNS messages with one question
			// Get the query fields according to the RFC specification
			struct QUERY query;
			if(ntohs(dns->q_count) == 1) {
				// Get NAME
				query.name = convertRFC2Name(reader,buf,&stop);
				reader = reader + stop;
				
				// Get QUESTION structure
				query.ques = (struct QUESTION*)(reader);
				reader = reader + sizeof(struct QUESTION);
				
				// Check question type. We only need to process A records.
				if(ntohs(query.ques->qtype) == 1) {
					printf("A record request.\n\n");
				} else {
					printf("NOT A record request!! Ignoring DNS message!\n");
					continue;
				}	
			} else {
				printf("\n\nDNS message must contain one question!! Ignoring DNS message!\n\n");
				continue;
			}

			#ifdef DEBUG
			printf("ID PEDIDO = %d\n", dns->id);
			#endif


			/***** VALIDAR PEDIDO *****/
			valido = 0;

			// 1. Verificar se e' localDomain (Se for, vai para a lista prioritaria)
			// TRABALHO DOS THREADS...
			if(strstr(query.name, ptr_config->localDomain) != NULL) {
				valido = 1;
				stat.pedidos_local += 1;
				insere_pedido(fila_prioridade, receber_info(query.name, &dest, dns->id, sockfd));
			} else if(ptr_config->flag % 2 == 0) {	// 2. Verificar se Ã© um dos dominios autorizados (ptr_config->domains[])
				for(i=0; i < ptr_config->n_domains; i++) {
					if(strstr(query.name, ptr_config->domains[i]) != NULL) {
						valido = 1;
						stat.pedidos_externos += 1;
						insere_pedido(fila_normal, receber_info(query.name, &dest, dns->id, sockfd));
						break;
					}
				}
			}

			#ifdef DEBUG
			imprimir_fila(fila_normal);
			imprimir_fila(fila_prioridade);
			#endif

			if(valido == 1) {
				#ifdef DEBUG
				printf("A ENVIAR SINAL PARA A THREAD...\n");
				#endif
				pthread_cond_signal(&cond);		
			} else {	
				if(ptr_config->flag % 2 != 0) {
					printf("Durante o modo de manuntencao apenas pedidos locais sao aceites!\n");
					printf("A enviar 0.0.0.0...\n");	
				} else {
					printf("Pedido negado!\nDe momento os dominios aceites sao:\n");
					for(i=0;i<ptr_config->n_domains;i++) {
						printf("-> Dominio %d = %s\n",i,ptr_config->domains[i]);
					}
				}
				sendReply(dns->id, query.name, inet_addr("0.0.0.0"), sockfd, dest);
				printf("\n\n-- Waiting for DNS message --\n\n");
			}

			if(valido == 0) {
				stat.pedidos_negados += 1;
			}
			stat.total_pedidos += 1;

			// Enviar dados das ESTATISTICAS para o named pipe aqui
			#ifdef DEBUG
			printf("A escrever estatisticas para o named pipe\n.");
			#endif
			write(np, &stat, sizeof(struct stats));
		}
		if (stats_process == 0 && cfg_process != 0) {	// STATS Process
			signal(SIGALRM, handler_alarm);

			Config* ptr;
			if ((ptr = shmat(shmID, NULL, 0)) == (Config *) -1) {
				perror("error in shmat");
				exit(1);
			}

			struct stats st;
			char* n_pipe = ptr_config->Named_pipe;

			if((r = open(n_pipe, O_RDONLY)) < 0) {
				perror("Child open Named pipe");
				exit(1);
			}

			read(r, &st, sizeof(struct stats));
			linha(st.data_hora_arranque);

			time_t clk2 = time(NULL);
			strcpy(st.data_info, ctime(&clk2));
			linha(st.data_info);

			t_p = st.total_pedidos;
			p_n = st.pedidos_negados;
			p_l = st.pedidos_local;
			p_e = st.pedidos_externos;
			strcpy(d_a, st.data_hora_arranque);
			strcpy(u_i, st.data_info);

			alarm(FREQ);
		}
	}

	return 0;
}

void token_mmap(Local local, char* data) {
	char* temp = data;

	char* token = strtok(temp, " ");

	int i;
	while(token != NULL) {
		Local aux = (Local) malloc(sizeof(Domains));
		//printf("%s\n", token);
		strcpy(aux->addr, token);
		token = strtok(NULL, "\n");
		strcpy(aux->ip, token);
		token = strtok(NULL, " ");

		//printf("%s\t%s\n", aux->addr, aux->ip);
		insere_domain(local, aux);
	}
}

void insere_domain(Local local, Local novo) {
	Local tmp = local;

	while(tmp->next != NULL)
		tmp = tmp->next;

	tmp->next = novo;
	novo->next = NULL;
}

Local cria_lista() {
	Local aux;
	aux = (Local) malloc(sizeof(Domains));

	if(aux != NULL) {
		aux->next = NULL;
	} else {
		fprintf(stderr, "Erro na alocacao de memoria");
	}
	return aux;
}

Fila cria_fila() {
	Fila aux;
	aux = (Fila) malloc(sizeof(Dados));

	if(aux != NULL) {
		aux->next = NULL;
	} else {
		fprintf(stderr, "Erro na alocacao de memoria");
	}
	return aux;
}

void imprimir_fila(Fila f) {
	Fila aux = f->next;
	char arr[16];
	printf("\n*** FILA ***\n");
	while(aux != NULL) {
		printf("[FILA] Query name = %s\n", aux->query);
		printf("Dominio = %s\nDNS ID = %d\n", inet_ntop(AF_INET, &(aux->origem.sin_addr.s_addr), arr, (socklen_t)1024), aux->dns_id);
		printf("\n");
		aux = aux->next;
	}
}

void eliminar_pedido(Fila fila) { // Eliminar o primeiro pedido.
	int rc = pthread_mutex_unlock(&mutex);
	Fila head, atual;

	head = fila;
	atual = fila->next;

	head->next = atual->next;
	free(atual);

	rc = pthread_mutex_lock(&mutex);
}

void insere_pedido(Fila f, Fila novo) {
	Fila tmp = f;

	while(tmp->next != NULL) {
		tmp = tmp->next;
	}

	tmp->next = novo;
	novo->next = NULL;
}

Fila receber_info(unsigned char* query, struct sockaddr_in* o, int dns_id, int socket) {
	Fila aux = (Fila) malloc(sizeof(Dados));

	strcpy(aux->query, query);
	aux->origem = *o;
	aux->socket = socket;
	aux->dns_id = dns_id;

	return aux;	
}