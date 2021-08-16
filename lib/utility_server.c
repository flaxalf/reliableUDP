#include "basic.h"

int port_config(){
				// Gestisce la configurazione della porta su cui il server è in ascolto
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
						perror("calloc in port_config");
						exit(EXIT_FAILURE);
					}
					printf("Insert new connection port: \n");
					if(fgets(conv, MAX_INPUT_LINE, stdin) == NULL) {
						perror("fgets in port_configt");
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
float prob_config(){
			//Gestisce la configurazione della probabilità di perdita
			char answer;
			float loss_prob;
			char *endptr;
 retry:
			printf("The default loss probability is set to: %f\n", LOSS_PROB);
			printf("Do you want to change it? (y/n)");
			answer = getchar();
      if(getchar()!='\n'){
        printf("Error, answer with y or n\n");
        while(getchar()!='\n');
        goto retry;
      };// rimuove '\n' da stdin
			if (answer == 'y'){
					printf("Insert new loss probability:\n");
					char *prob;
					if ((prob = calloc(MAX_INPUT_LINE, sizeof(char))) == NULL) {
						perror("calloc");
						exit(EXIT_FAILURE);
					}
					if(fgets(prob, MAX_INPUT_LINE, stdin) == NULL) {
						perror("fgets loss prob");
						exit(EXIT_FAILURE);
					}
					errno = 0;
					loss_prob = strtof(prob, &endptr);
					if((errno == ERANGE && (loss_prob == HUGE_VALF ||
									loss_prob == HUGE_VALL)) || (errno != 0 && loss_prob == 0)) {
						fprintf(stderr, "Invalid value\n");
						goto retry;
					}
					if(endptr == prob) {
						fprintf(stderr, "No digits were found\n");
						goto retry;
					}
					//controlla che sia stato inserito un valore valido
					if(loss_prob <= 0 || loss_prob > 1){
						fprintf(stderr, "Invalid value [0 - 1]\n");
						goto retry;
					}
					free(prob);
			} else if (answer == 'n'){
					loss_prob = LOSS_PROB;
			} else {
					printf("Error, answer with 'y' or 'n'\n");
					goto retry;
			}
			return loss_prob;
}
int window_config(){
	//Gestisce la configurazione della finestra di trasmissione
	int window;
	char answer;
 retry:
	printf("The default window is set to: %d\n", WINDOW);
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
			perror("calloc in window_config");
			exit(EXIT_FAILURE);
		}
		printf("Insert new window size: \n");
		if(fgets(conv, MAX_INPUT_LINE, stdin) == NULL) {
			perror("fgets in window_config");
			exit(EXIT_FAILURE);
		}
		errno = 0;
		long window_in = strtol(conv, &endptr, 10);
		if((errno == ERANGE && (window_in == LONG_MAX || window_in == LONG_MIN))
				|| (errno != 0 && window_in == 0)) {
			fprintf(stderr, "Invalid value\n");
			goto retry;
		}
		if(endptr == conv) {
			fprintf(stderr, "No digits were found\n");
			goto retry;
		}
		//controllo se il valore inserito è valido
		if(window_in <= 0){
			fprintf(stderr, "Invalid value, must be > 0\n");
			goto retry;
		}
		window = (int) window_in;
		free(conv);
	} else if (answer == 'n'){
		window = WINDOW;
	} else {
		printf("Error, answer with 'y' or 'n'\n");
		goto retry;
	}
	return window;
}
int timeout_config(){
	//Gestisce la configurazione del timeout
  int timeout;
	char answer;
 retry:
	printf("The default timeout is set to: %d milli-sec\n", TIMEOUT_PKT / 1000);
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
			perror("calloc in timeout_config");
			exit(EXIT_FAILURE);
		}
    //la funzione che gestisce il timeout lo fa diventare dinamico se uguale a 0
		printf("Insert timeout value in ms or 0 if you want it to be dinamic:\n");
		if(fgets(conv, MAX_INPUT_LINE, stdin) == NULL) {
			perror("fgets window_config");
			exit(EXIT_FAILURE);
		}
		errno = 0;
		long timeout_in = strtol(conv, &endptr, 10) / 1000;
		if((errno == ERANGE && (timeout_in== LONG_MAX || timeout_in == LONG_MIN))
				|| (errno != 0 && timeout_in == 0)) {
			fprintf(stderr, "Invalid value\n");
			goto retry;
		}
		if(endptr == conv) {
			fprintf(stderr, "No digits were found\n");
			goto retry;
		}
    if(timeout_in != 0 &&(timeout_in < TIMEOUT_PKT || timeout_in> MAX_TIMEOUT)){
      printf("Invalide value, it must be between %d and %d or 0 for dinamic\n",
			 																								MIN_TIMEOUT, MAX_TIMEOUT);
      goto retry;
    }
		timeout = (int) timeout_in;
		free(conv);
	} else if (answer == 'n'){
		timeout = TIMEOUT_PKT;
	} else {
		printf("Error, answer with 'y' or 'n'\n");
		goto retry;
	}
	return timeout;
}

int reliable_accept(int sockfd, struct sockaddr_in *cliaddr, socklen_t *len,
	 									int window, float loss_prob, int timeout){
	//Gestisce la connesione con i client tramite three way handshake
	t_pkt *syn_pkt, *synack_pkt, *ack_pkt;
	int new_sockfd;
	pid_t pid;

	if((syn_pkt = (t_pkt *) calloc(1, sizeof(*syn_pkt))) == NULL) {
		perror("calloc in reliable_accept");
		exit(EXIT_FAILURE);
	}
	do {
		errno = 0;
		if(recvfrom(sockfd, syn_pkt, sizeof(*syn_pkt), 0,
										(struct sockaddr *)cliaddr, len) < 0 && errno != EAGAIN) {
			if(errno == EINTR) {
				/* Caso in cui siamo stati interrotti in quanto un figlio ha teterminato
				* il suo ciclo di vita, rientreremo in rcvfrom
				*/
				continue;
			}
			perror("recvfrom - listening incoming connection error");
			exit(EXIT_FAILURE);
		}
		// fino a quando non riceve un syn, ascolto
	} while(errno == EAGAIN || errno == EINTR);
	if(syn_pkt->type == SYN) {
		free(syn_pkt);
		if ((pid = fork()) < 0) {
			perror("fork in reliable_accept");
			exit(EXIT_FAILURE);
		}
		if(pid != 0){
			// il padre esce
			return 0;
		}
		// child chiude la socket di connessione
		if(close(sockfd) < 0){
			perror("error in child closing connection socket");
			_exit(EXIT_FAILURE);
		}
		srand(time(NULL)); //ogni processo figlio ha il suo seed
		new_sockfd = create_socket(timeout);
		if((synack_pkt = calloc(1, sizeof(*synack_pkt))) == NULL) {
			perror("calloc in reliable_accept");
			_exit(EXIT_FAILURE);
		}
		synack_pkt->type = SYNACK;
		// synack_pkt in data : [window];[loss_prob];[timeout]
		sprintf(synack_pkt->data, "%d", window);
		strcat(synack_pkt->data, ";");
		sprintf(synack_pkt->data + strlen(synack_pkt->data), "%f", loss_prob);
		strcat(synack_pkt->data, ";");
		sprintf(synack_pkt->data+strlen(synack_pkt->data), "%d", timeout);

		if((ack_pkt = calloc(1, sizeof(*ack_pkt))) == NULL) {
			perror("calloc ack_pkt");
			_exit(EXIT_FAILURE);
		}
		for(int tries = 5; tries > 0; --tries){
			rand_send(new_sockfd, synack_pkt, cliaddr, loss_prob);
			errno = 0;
			if(recvfrom(new_sockfd, ack_pkt, sizeof(*ack_pkt), 0,
			 								(struct sockaddr *)cliaddr, len) < 0 && errno != EAGAIN) {
				perror("recvfrom in reliable_accept");
				_exit(EXIT_FAILURE);
			}
			if(errno == EAGAIN){
				continue;
			}
			if(ack_pkt->type == ACK){
				free(ack_pkt);
				free(synack_pkt);
				return new_sockfd;
			}
		}
		free(ack_pkt);
		free(synack_pkt);
	} else {
		_exit(EXIT_FAILURE);
	}
	_exit(EXIT_FAILURE);
}

void server_list_work(int sockfd, struct sockaddr_in *cliaddr,
	 										int window, float loss_prob) {
  // Gestisce l'invio della lista di file presenti nel server
  char *list_files[MAX_FILE_LIST], *buff, *path;
  int num_files, i, fd;
	short int j;

  num_files = files_from_folder(list_files);

	if((buff = calloc(1, (MAX_NAMEFILE_LEN+1) * num_files )) == NULL){
		perror("calloc buff in list");
		_exit(EXIT_FAILURE);
	}
	//salvo i nomi dei file in un buffer
  size_t len;
  int position = 0;
  for(i = 0; i < num_files; ++i){
    len = strlen(list_files[i]);
    strncpy(buff + position, list_files[i], MAX_NAMEFILE_LEN);
    position += len;
    strncpy(buff + position, "\n", 1);
    position ++;
  }
	*(buff + position) = '\0';
	path = create_path(TEMP_FOLDER, "ser_temp.txt");

	for(i = 0; i < MAX_FILE_LIST; ++i){
		free(list_files[i]);
	}
	for(j = 1; j < 10; ++j) {
		if(check_file(path) == -1) {
			//non esiste, il file con tale nome può essere salvato senza rinominare
			break;
		} else {
			snprintf(path,strlen(TEMP_FOLDER) + 14, "%sser_temp%d.txt",TEMP_FOLDER,j);
		}
	}
	//creo un file temporaneo in cui copio buffer
	if((fd = open(path, O_WRONLY | O_CREAT, 0666)) < 0){
		perror("open in server_list_work");
		_exit(EXIT_FAILURE);
	}
	if(write_block(fd, buff, strlen(buff)) < 0) {
		perror("write_block in list work");
		_exit(EXIT_FAILURE);
	}
	free(buff);
	if(close(fd) < 0) {
		perror("close file in server_list_work");
		_exit(EXIT_FAILURE);
	}
	//gestione dell'invio tramite selective repeat
	sr_snd_side(sockfd, cliaddr, path, window, loss_prob);

	if(unlink(path) < 0){
		perror("unlink in server_list_work");
		_exit(EXIT_FAILURE);
	}
	free(path);
}
void server_get_work(int sockfd, struct sockaddr_in *cliaddr, t_pkt *pkt,
	 									 int window, float loss_prob) {
	// Gestisce l'invio del file richiesto dal client
	char *path;
	path = create_path(SERVER_FOLDER, pkt->data);

	if(check_file(path) == -1){
		//il file non esiste, devo rispondere al client
		t_pkt *error_pkt;
		if((error_pkt = calloc(1, sizeof(*error_pkt))) == NULL) {
			perror("calloc in server_get_work");
			_exit(EXIT_FAILURE);
		}
		error_pkt->type = ERROR;
		strcpy(error_pkt->data, "File does not exist");

		rand_send(sockfd, error_pkt, cliaddr, loss_prob);
		free(path);
		free(error_pkt);
		return;
	}
	//gestisce l'invio tramite selective repeat
	sr_snd_side(sockfd, cliaddr, path, window, loss_prob);
	free(path);
}
void server_put_work(int sockfd, struct sockaddr_in *cliaddr, t_pkt *pkt,
	 									 int window, float loss_prob) {
  //Gestisce la ricezione dei file
	short int i;
	char *initial_path_no_ext, *path, *file_ext;
	t_pkt *ack_pkt;
	if((ack_pkt = calloc(1, sizeof(*ack_pkt))) == NULL) {
		perror("calloc server_put_work");
		exit(EXIT_FAILURE);
	}
	if((initial_path_no_ext = calloc(1,strlen(SERVER_FOLDER) +
	 																									MAX_INPUT_LINE)) == NULL) {
		perror("callocin server_put_work");
		_exit(EXIT_FAILURE);
	}
	if((file_ext = calloc(1, 10)) == NULL){//10 come grandezza massima estensione
		perror("calloc in server_put_work");
		_exit(EXIT_FAILURE);
	}

	path = create_path(SERVER_FOLDER, pkt->data);

	if(	strlen(path) - strlen(strrchr(path, '.')) != 0){
		// non ho preso il 1° punto del path (./folder)
		strncpy(file_ext, strrchr(path, '.'), 10);
	}
	strncpy(initial_path_no_ext, path, sizeof(char)*(strlen(path) -
																														strlen(file_ext)));

	for(i = 1; i <= 11; ++i) {
		if(i == 11){
			ack_pkt->type = ERROR;
			strcpy(ack_pkt->data,"File name not available, choose another name");
			rand_send(sockfd, ack_pkt, cliaddr, loss_prob);
			free(initial_path_no_ext);
			free(ack_pkt);
			free(file_ext);
			free(path);
			return;
		}
		if(check_file(path) == -1) {
			//non esiste, il file con tale nome può essere salvato
			break;
		} else {
			snprintf(path, strlen(initial_path_no_ext) + 3 + strlen(file_ext) + 1 ,
			 														"%s(%d)%s", initial_path_no_ext, i, file_ext);
		}
	}
	ack_pkt->type = ACK;
	ack_pkt->n_seq = -1; //indica ack di put evitando errori in sr_rcv_side
	rand_send(sockfd, ack_pkt, cliaddr, loss_prob);

	free(initial_path_no_ext);
	free(ack_pkt);
	free(file_ext);
	sr_rcv_side(sockfd, cliaddr, path, window, loss_prob);
	free(path);
}
void server_quit_work(int sockfd, struct sockaddr_in *cliaddr) {
  //Gestisce la chiusura della connessione con il client
	if(close(sockfd) < 0){
		perror("close sockfd");
		_exit(EXIT_FAILURE);
	}
	free(cliaddr);
	printf("Connection closed %d\n", getpid());
	_exit(EXIT_SUCCESS);
}
