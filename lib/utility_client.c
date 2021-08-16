#include "basic.h"

int port_config(){
				//gestisce la configurazione della porta contattata dal client
				int conn_port;
				char answer;
	retry:
				printf("The default connection port is set to: %d\n", SERV_PORT);
				printf("Do you want to change it? (y/n)");
				answer = getchar();
				if(getchar()!='\n'){
					printf("Error, answer with y or n\n");
					while(getchar()!='\n');
					goto retry;
				}; // rimuove '\n' da stdin
				if (answer == 'y'){
					char *endptr, *conv;
					if((conv = calloc(MAX_INPUT_LINE, sizeof(char))) == NULL) {
						perror("calloc port in port_config");
						exit(EXIT_FAILURE);
					}
					printf("Insert new connection port: \n");
					if(fgets(conv, MAX_INPUT_LINE, stdin) == NULL) {
						perror("fgets in port_config");
						exit(EXIT_FAILURE);
					}
					errno = 0;
					long port_in = strtol(conv, &endptr, 10);
					if((errno == ERANGE && (port_in == LONG_MAX || port_in == LONG_MIN))
							|| (errno != 0 && port_in == 0)) {
						fprintf(stderr, "Invalid value\n");
						goto retry;
					}
					if(endptr == conv) {
						fprintf(stderr, "No digits were found\n");
						goto retry;
					}
					// controlla se la porta è valida
					if(port_in > 0 && port_in < MAX_PORT) {
						conn_port = (int) port_in;
						return conn_port;
					} else {
						printf("Invalid value [0 - 65535]\n");
						goto retry;
					}
					free(conv);
				} else if (answer == 'n'){
					conn_port = SERV_PORT;
				} else {
					printf("Error, answer with 'y' or 'n'\n");
					goto retry;
				}
				return conn_port;
}
char* ip_config(){
			//gestisce la configurazione dell'IP contattato dal client
			char answer;
			char* server_ip;
			server_ip = calloc(MAX_INPUT_LINE, sizeof(char));
			if(server_ip == NULL){
				perror("calloc in ip_config");
				exit(EXIT_FAILURE);
			}

	retry:
			printf("The default server IP is set to: %s\n", SERVIP);
			printf("Do you want to change it? (y/n)");
			answer = getchar();
			if(getchar()!='\n'){
				printf("Error, answer with y or n\n");
				while(getchar()!='\n');
				goto retry;
			}; // rimuove '\n' da stdin
			if (answer == 'y'){
				int res;
				void *addr;
				addr = calloc(1, sizeof(struct in_addr));
				if(addr == NULL){
					perror("calloc server ip");
					exit(EXIT_FAILURE);
				}
				printf("Insert new IP: \n");
				if(fgets(server_ip, MAX_INPUT_LINE, stdin) == NULL) {
					perror("fgets in ip_config");
					exit(EXIT_FAILURE);
				}
				size_t size = strlen(server_ip);
				server_ip[size-1] = '\0';
				res = inet_pton(AF_INET, server_ip, addr);
				if( res <= 0) {
					if (res == 0){
						printf("Ip format not valid, try again!\n");
						goto retry;
					} else{
						perror("inet_pton in ip_config");
						exit(EXIT_FAILURE);
					}
				}
				free(addr);

			} else if(answer == 'n') {
				server_ip = SERVIP;
			} else {
				printf("Please, answer with 'y' or 'n'\n");
				goto retry;
			}
			return server_ip;
}

int reliable_connection(int sockfd, struct sockaddr_in *servaddr,
	 											int *window, float *loss_prob, int *timeout) {
	//effettua la connessione al server tramite un three way handshake
	int tries;
	char synack = 0;
	t_pkt *syn_pkt, *synack_pkt, *ack_pkt;
	char *endptr;

	if((syn_pkt = calloc(1, sizeof(*syn_pkt))) == NULL) {
		perror("calloc in reliable_connection");
		exit(EXIT_FAILURE);
	}
	syn_pkt->type = SYN;

	if((synack_pkt = calloc(1, sizeof(*synack_pkt))) == NULL) {
		perror("calloc in reliable_connection");
		exit(EXIT_FAILURE);
	}

	if((ack_pkt = calloc(1, sizeof(*ack_pkt))) == NULL) {
		perror("calloc in reliable_connection");
		exit(EXIT_FAILURE);
	}
	ack_pkt->type = ACK;

	for(tries = 5; tries > 0; --tries) {
		rand_send(sockfd, syn_pkt, servaddr, LOSS_PROB);
		socklen_t size = sizeof(*servaddr);
		errno = 0;
		if(recvfrom(sockfd, synack_pkt, sizeof(*synack_pkt), 0,
								(struct sockaddr *)servaddr, &size) < 0 && errno != EAGAIN){
			perror("rcvfrom in reliable_connection");
			exit(EXIT_FAILURE);
		}
		if(synack_pkt->type == SYNACK) {
			// synack_pkt in data : [window];[loss_prob];[timeout]
			*window = (int) strtol(synack_pkt->data, &endptr, 10);
			*loss_prob = (float) strtof(endptr+1, &endptr);
			*timeout = (int) strtol(endptr+1, &endptr, 10);
			if (*endptr != '\0'){
				fprintf(stderr, "Wrong data received\n");
				exit(EXIT_FAILURE);
			}
			/*Ho ricevuto il synack, posso uscire dal ciclo trasmissione syn/synack
			* e procedere alla trasmissione dell'ack */
			synack = 1;
			break;
		} else {
			/* Necessario ritrasmettere il syn nel caso in cui abbia ricevuto qualcosa
			* di diverso dal synack o nel caso in cui fosse terminato il timeout
			* nella rcvfrom
			*/
		}
	}
	free(syn_pkt);
	if (synack) {
			set_timeout(sockfd, *timeout);
			/* Se ricevessi di nuovo un synack, vorrebbe dire che l'ack è stato perso,
			 * quindi devo trasmetterlo di nuovo per concludere la connessione
			 */
			char again = 1;
			while (again) {
				//mando l' ack
				rand_send(sockfd, ack_pkt, servaddr, *loss_prob);

				socklen_t size = sizeof(*servaddr);
				for (tries = 5; tries > 0; --tries){
					// il client attende un eventuale synack ripetuto da parte del server
					errno = 0;
					if(recvfrom(sockfd, synack_pkt, sizeof(*synack_pkt), 0,
					 				(struct sockaddr *)servaddr, &size) < 0 && errno != EAGAIN){
						perror("rcvfrom in reliable_connection");
						exit(EXIT_FAILURE);
					}
					if (errno == EAGAIN) {
						again = 0;
					} else {
						again = 1;
						break;
					}
				}
			}
			free(ack_pkt);
			free(synack_pkt);
			return 0;
	}

		free(ack_pkt);
		free(synack_pkt);
		return -1;
}

int menu_cli() {
		//gestisce l'inserimento di comandi da stdin da parte dell'utente
		char *cmd;
		short int invalid_cmd = -1;

		if((cmd = malloc(MAX_INPUT_LINE)) == NULL) {
			perror("malloc in menu_cli");
			exit(EXIT_FAILURE);
		}
		if(fgets(cmd, MAX_INPUT_LINE, stdin) == NULL) {
			perror("fgets in menu_cli");
			exit(EXIT_FAILURE);
		}

		cmd[strlen(cmd) - 1] = '\0';
		if (strcasecmp(cmd, STR_LIST) == 0 || strcasecmp(cmd, CHR_LIST) == 0) {
			free(cmd);
			return LIST;
		}
		if (strcasecmp(cmd, STR_GET) == 0 || strcasecmp(cmd, CHR_GET) == 0) {
			free(cmd);
			return GET;
		}
		if (strcasecmp(cmd, STR_PUT) == 0 || strcasecmp(cmd, CHR_PUT) == 0) {
			free(cmd);
			return PUT;
		}
		if (strcasecmp(cmd, STR_QUIT) == 0 || strcasecmp(cmd, CHR_QUIT) == 0) {
			free(cmd);
			return QUIT;
		}
		if (strcasecmp(cmd, STR_HELP) == 0 || strcasecmp(cmd, CHR_HELP) == 0) {
			free(cmd);
			return HELP;
		}

		free(cmd);

		return invalid_cmd;
	}

void client_list_work(int sockfd, struct sockaddr_in *servaddr,
	 										int window, float loss_prob, int timeout) {
	char *path, *cat_cmnd;
	short int i;
	pid_t pid;
	t_pkt *list_pkt;
	if((list_pkt = calloc(1, sizeof(*list_pkt))) == NULL) {
		perror("calloc in client_list_work");
		exit(EXIT_FAILURE);
	}
	list_pkt->type = LIST;
	if((cat_cmnd = calloc(1, 30)) == NULL) { //cat cli_tempN.txt
		perror("callloc in client_list_work");
		exit(EXIT_FAILURE);
	}
	path = create_path(TEMP_FOLDER, "cli_temp.txt");

	for(i = 1; i <=11;++i) {
		if(i == 11){
			printf("Try again\n");
			free(list_pkt);
			if(unlink(path) < 0){
				perror("unlink temp.txt");
				_exit(EXIT_FAILURE);
			}
			free(cat_cmnd);
			free(path);
			if (close(sockfd) < 0) {
		    perror("close in client_list_work");
		    exit(EXIT_FAILURE);
		  }
			return;
		}
		if(check_file(path) == -1) {
			//non esiste, il file con tale nome può essere salvato senza rinominare
			break;
		} else {
			snprintf(path, strlen(TEMP_FOLDER)+15,"%scli_temp%d.txt", TEMP_FOLDER, i);
		}
	}

	if ((pid = fork()) < 0) {
		perror("fork in client_list_work");
		exit(EXIT_FAILURE);
	}
	if(pid != 0){
		return;
	}
	sockfd = create_socket(timeout);

	srand(time(NULL)); // imposto seed per il nuovo figlio
	rand_send(sockfd, list_pkt, servaddr, loss_prob);
	//ricezione tramite selective repeat
	free(list_pkt);
	sr_rcv_side(sockfd, servaddr, path, window, loss_prob);
	strcpy (cat_cmnd, "cat ");
	strcat(cat_cmnd, path);
	printf("LIST:\n");
	if(system(cat_cmnd) < 0){
		fprintf(stderr, "Error in 'system' call\n");
		_exit(EXIT_FAILURE);
	}
	printf("\n");
	if(unlink(path) < 0){
		perror("unlink temp.txt");
		_exit(EXIT_FAILURE);
	}
	free(cat_cmnd);
	free(path);
	if (close(sockfd) < 0) {
    perror("close in client_list_work");
    exit(EXIT_FAILURE);
  }
	_exit(EXIT_SUCCESS);
}
void client_get_work(int sockfd, struct sockaddr_in *servaddr,
	 									int window, float loss_prob, int timeout) {
  // Gestisce il download dal server di un file scelto dall'utente
	char *initial_path_no_ext, *path, *file_ext, *file_name;
  short int i;
  t_pkt *get_pkt;
	pid_t pid;
	if((initial_path_no_ext = calloc(1, strlen(CLIENT_FOLDER) + MAX_INPUT_LINE))
														== NULL){
		perror("calloc in client_get_work");
		exit(EXIT_FAILURE);
	}
  if((file_name = calloc(1, MAX_INPUT_LINE)) == NULL) {
    perror("calloc in client_get_work");
    exit(EXIT_FAILURE);
  }
	if((file_ext = calloc(1, 10)) == NULL) {
    perror("calloc in client_get_work");
    exit(EXIT_FAILURE);
  }
	if((get_pkt = calloc(1, sizeof(*get_pkt))) == NULL) {
		perror("calloc in client_get_work");
		exit(EXIT_FAILURE);
	}
	get_pkt->type = GET;
  printf("Which file do you want to get?\n");
  if(fgets(file_name, MAX_INPUT_LINE, stdin) == NULL) {
    perror("fgets in client_get_work");
    exit(EXIT_FAILURE);
  }
  file_name[strlen(file_name) - 1] = '\0';

	if ((pid = fork()) < 0) {
		perror("fork in client_get_work");
		exit(EXIT_FAILURE);
	}
	if(pid != 0){
		return;
	}
	srand(time(NULL)); // imposto seed per il nuovo figlio
	sockfd = create_socket(timeout);
  strncpy(get_pkt->data, file_name, MAX_INPUT_LINE);

	path = create_path(CLIENT_FOLDER, file_name);
	if(strrchr(file_name, '.') != NULL){
		strncpy(file_ext, strrchr(file_name, '.'), 10);
	}
	strncpy(initial_path_no_ext, path, sizeof(char) *
																						(strlen(path) - strlen(file_ext)));

	for(i = 1; i <= 11 ;++i) {
		if(i == 11){
			printf("File name not available\n");
			free(file_name);
			free(initial_path_no_ext);
			free(file_ext);
			free(path);
			free(get_pkt);
			if (close(sockfd) < 0) {
				perror("close in client_get_work");
				exit(EXIT_FAILURE);
			}
			_exit(EXIT_SUCCESS);
		}
		if(check_file(path) == -1) {
			//non esiste, il file con tale nome può essere salvato senza rinominare
			break;
		} else {
			//Necessario rinominare il file aggiungendo un indice
			snprintf(path,strlen(initial_path_no_ext) + 3 + strlen(file_ext) +1,
			 														"%s(%d)%s", initial_path_no_ext, i, file_ext);
		}
	}
	rand_send(sockfd, get_pkt, servaddr, loss_prob);
	free(get_pkt);
  //ricezione del file tramite selective repeat
	free(file_name);
	free(initial_path_no_ext);
	free(file_ext);
  sr_rcv_side(sockfd, servaddr, path, window, loss_prob);
  free(path);
	if(close(sockfd) < 0){
    perror("close in client_get_work");
    exit(EXIT_FAILURE);
  }
	_exit(EXIT_SUCCESS);
}
void client_put_work(int sockfd, struct sockaddr_in *servaddr,
	 									 int window, float loss_prob, int timeout) {
  // Gestisce la richiesta di upload di un file sul server
	char *file_name, *path;
	pid_t pid;
	t_pkt *put_pkt, *ack_pkt;
	socklen_t size = sizeof(*servaddr);
	if((file_name = calloc(1, MAX_INPUT_LINE)) == NULL) {
		perror("calloc in client_put_work");
		exit(EXIT_FAILURE);
	}
	printf("Which file do you want to put?\n");
	if(fgets(file_name, MAX_INPUT_LINE, stdin) == NULL) {
		perror("fgets in client_put_work");
		exit(EXIT_FAILURE);
	}
	file_name[strlen(file_name) - 1] = '\0';
	if ((pid = fork()) < 0) {
		perror("fork in client_get_work");
		exit(EXIT_FAILURE);
	}
	if(pid != 0){
		return;
	}
	sockfd = create_socket(timeout);
	srand(time(NULL)); // imposto seed per il nuovo figlio

	path = create_path(CLIENT_FOLDER, file_name);
	if(check_file(path) == -1){
		printf("The file does not exist\n");
		return;
	}

	if((put_pkt = calloc(1, sizeof(*put_pkt))) == NULL) {
		perror("calloc in client_put_work");
		exit(EXIT_FAILURE);
	}
	put_pkt->type = PUT;
	strncpy(put_pkt->data, file_name, MAX_INPUT_LINE);
	free(file_name);

	if((ack_pkt = calloc(1, sizeof(*ack_pkt))) == NULL) {
		perror("calloc in client_put_work");
		exit(EXIT_FAILURE);
	}
	for(int tries = 5; tries > 0; --tries){
		rand_send(sockfd, put_pkt, servaddr, loss_prob);

		errno = 0;
		if(recvfrom(sockfd, ack_pkt, sizeof(*ack_pkt), 0,
		 						(struct sockaddr *)servaddr, &size) < 0 && errno != EAGAIN) {
			perror("recvfrom in client_put_work");
			exit(EXIT_FAILURE);
		}
		if(errno == EAGAIN){
			continue;
		}
		if(ack_pkt->type == ERROR){
			printf("%s\n", ack_pkt->data);
			free(put_pkt);
			free(ack_pkt);
			free(path);
			if (close(sockfd) < 0) {
		    perror("closw in client_put_work");
		    exit(EXIT_FAILURE);
		  }
			_exit(EXIT_SUCCESS);
		}
		if(ack_pkt->type == ACK){
			break;
		}
  }
	free(put_pkt);
	free(ack_pkt);
	//gestione dell'invio con selective repeat
	sr_snd_side(sockfd, servaddr, path, window, loss_prob);
	free(path);
	if (close(sockfd) < 0) {
    perror("close in client_put_work");
    exit(EXIT_FAILURE);
  }
	_exit(EXIT_SUCCESS);
}
void client_quit_work(int sockfd, struct sockaddr_in *servaddr, float loss_prob) {
  // Gestisce la chiusura della connessione con il server
	printf("Starting closing procedure...\n");
	t_pkt *quit_pkt;

	if((quit_pkt = calloc(1, sizeof(*quit_pkt))) == NULL) {
		perror("malloc in quit_work");
		exit(EXIT_FAILURE);
	}
	quit_pkt->type = QUIT;

	rand_send(sockfd, quit_pkt, servaddr, loss_prob);
	rand_send(sockfd, quit_pkt, servaddr, loss_prob);
	rand_send(sockfd, quit_pkt, servaddr, loss_prob);

	printf("-> Quit request sent\n");

	if (close(sockfd) < 0) {
    perror("close in client_quit_work");
    exit(EXIT_FAILURE);
  }
	printf("\nConnection with server closed successfully.\n");
	free(quit_pkt);
}
