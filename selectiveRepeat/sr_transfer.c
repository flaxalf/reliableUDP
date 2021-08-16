#include "../lib/basic.h"

int get_timeout(int sockfd){
	// Restituisce l'attuale valore del timeout della socket
	struct timeval *time;
	int dimension = sizeof(*time);
	if((time = calloc(1, dimension)) == NULL) {
		perror("calloc get_timeout");
		_exit(EXIT_FAILURE);
	}
	if(getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(void*) time,(socklen_t*) &dimension) < 0){
		perror("getsockopt get_timeout");
		_exit(EXIT_FAILURE);
	}
	free(time);
	return time->tv_usec;
}
void set_timeout(int sockfd, int timeout) {
	// Imposta il timeout della socket in microsecondi
	struct timeval *time;
	if((time = calloc(1, sizeof(*time))) == NULL) {
		perror("calloc set_timeout");
		exit(EXIT_FAILURE);
	}
	if(timeout != 0){
		time->tv_usec = timeout;
		if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
			 																		(char *) time, sizeof(*time)) < 0) {
			perror("setsockopt set_timeout");
			exit(EXIT_FAILURE);
		}
	} else{
		time->tv_usec = TIMEOUT_PKT;
		if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
			 																		(char *) time, sizeof(*time)) < 0) {
			perror("setsockopt set_timeout");
			exit(EXIT_FAILURE);
		}
	}
	free(time);
}
void set_timeout_sec(int sockfd, int timeout) {
	//Imposta il timeout della socket in secondi
	struct timeval *time;
	if((time = calloc(1, sizeof(*time))) == NULL) {
		perror("calloc set_timeout_sec");
		exit(EXIT_FAILURE);
	}
	time->tv_sec = timeout;
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
		 																				(char*) time, sizeof(*time)) < 0) {
		perror("setsockopt set_timeout_sec");
		exit(EXIT_FAILURE);
	}
	free(time);
}
void decrease_timeout(int sockfd){
	// Decrementa il valore attuale del timeout in caso di timeout adattativo
	int timeout;
	if(static_time == 0){
		timeout = get_timeout(sockfd);
		if(timeout >= MIN_TIMEOUT + TIME_UNIT){
			set_timeout(sockfd, timeout - TIME_UNIT);
		}
	}
}
void increase_timeout(int sockfd){
	//Incrementa l'attuale valore del timeout in caso di timeout adattativo
	int timeout;
	if(static_time == 0){
		timeout = get_timeout(sockfd);
		if(timeout >= MAX_TIMEOUT - TIME_UNIT){
			set_timeout(sockfd, timeout + TIME_UNIT);
		}
	}
}
int create_socket(int timeout){
	//Crea una nuova socket, assegnandole una porta libera
	struct sockaddr_in *new_addr;
	int new_sockfd;
	if((new_addr = calloc(1, sizeof(*new_addr))) == NULL) {
		perror("calloc in create_socket");
		_exit(EXIT_FAILURE);
	}
	new_addr->sin_family = AF_INET;
	new_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	new_addr->sin_port = htons(0);
	// La nuova porta è assegnata al figlio automaticamente dal kernel siccome
	// abbiamo messo 0. Verrà scelta una porta tra quelle non utilizzate.
	if((new_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("socket in create_socket");
		_exit(EXIT_FAILURE);
	}
	if(bind(new_sockfd, (struct sockaddr*) new_addr, sizeof(*new_addr)) < 0){
		perror("bind in create_socket");
		_exit(EXIT_FAILURE);
	}
	free(new_addr);
	set_timeout(new_sockfd, timeout);
	return new_sockfd;
}

void rand_send(int sockfd,t_pkt *pkt,struct sockaddr_in *addr, float loss_prob){
	//In modo randomico invia o non invia il pacchetto
	float random_prob = (float)rand() / (float)RAND_MAX;
	if(random_prob > loss_prob) {
		if(sendto(sockfd, pkt, sizeof(*pkt), 0,
		 															(struct sockaddr *)addr, sizeof(*addr)) < 0) {
			perror("sendto rand_send");
			exit(EXIT_FAILURE);
		}
	}
}

void sr_rcv_side(int sockfd, struct sockaddr_in *addr, char *file_name,
	 																								int window, float loss_prob){
	int fd, left_range = 0, right_range = window - 1; //se window = 3 -> [0, 2]
	int pkt_size_data = PKT_SIZE - (sizeof(short int) + sizeof(int));
	int capacity = 2 * window + 1, cnt_inactive = 0, tries;
	float tot_time = 0, speed;
	socklen_t size = sizeof(*addr);
	short int transferring = 1;
	t_pkt buff_pkts[capacity];
	t_pkt *rcv_pkt, *ack_pkt;
	//file size impostato ad un valore iniziale arbitrario
	long file_size = 1000000000 , written = 0, total_written = 0, size_to_write;
	char *endptr, *buff;
	struct timeval *start, *end;

	//ALLOCAZIONE STRUTTURE e APERTURA FILE
	if((start = calloc(1, sizeof(*start))) == NULL) {
		perror("calloc sr_rcv_side");
		_exit(EXIT_FAILURE);
	}
	if((end = calloc(1, sizeof(*end))) == NULL) {
		perror("calloc sr_rcv_side");
		_exit(EXIT_FAILURE);
	}
	if((rcv_pkt = calloc(1, sizeof(*rcv_pkt))) == NULL) {
		perror("calloc sr_rcv_side");
		_exit(EXIT_FAILURE);
	}
	if((ack_pkt = calloc(1, sizeof(*ack_pkt))) == NULL) {
		perror("calloc sr_rcv_side");
		_exit(EXIT_FAILURE);
	}
	ack_pkt->type = ACK;
	if((buff = calloc(WRITABLE_BUFF, pkt_size_data)) == NULL) {
		perror("calloc sr_rcv_side");
		_exit(EXIT_FAILURE);
	}

	if((fd = open(file_name, O_WRONLY | O_CREAT, 0666)) < 0){
		perror("open sr_rcv_side");
		_exit(EXIT_FAILURE);
	}

  //Inizializzo il buff di pkt
  for (int i = 0; i < capacity; i++) {
    buff_pkts[i].n_seq = -1;
  }
	gettimeofday(start, NULL);
	printf("Start of the transfer of %s\n", file_name);
	//ROUTINE
	while(transferring){
		errno = 0;
		memset(rcv_pkt, 0, sizeof(*rcv_pkt));
		//mi metto in attesa di ricezione di un pacchetto dati
		if(recvfrom(sockfd, rcv_pkt, sizeof(*rcv_pkt), 0,
											(struct sockaddr *)addr, &size) < 0 && errno != EAGAIN) {
			perror("recvfrom in sr_rcv_side");
			_exit(EXIT_FAILURE);
		}
		if(errno == EAGAIN) {
			// scade il timeout, non sto ricevendo nulla
			cnt_inactive++;
			if(cnt_inactive == MAX_INACTIVE_TRIES){
				printf("Inactive sender\n\n");
				if(unlink(file_name) < 0) {
					perror("unlink in sr_rcv_side");
					_exit(EXIT_FAILURE);
				}
				free(end);
				free(start);
				free(rcv_pkt);
				free(file_name);
				_exit(EXIT_FAILURE);
			}
			increase_timeout(sockfd);
			continue;
		}
		switch (rcv_pkt->type) {
			case DATA_SIZE:
				//ricevo dimensione del file
				cnt_inactive = 0;
				decrease_timeout(sockfd);
				ack_pkt->n_seq = rcv_pkt->n_seq;
				if(left_range == 0){
					errno = 0;
					file_size = strtol(rcv_pkt->data, &endptr, 10);
					if((errno==ERANGE && (file_size == LONG_MAX || file_size == LONG_MIN))
																						|| (errno != 0 && file_size == 0)) {
						perror("strtol in sr_snd_side");
						_exit(EXIT_FAILURE);
					}
					left_range++;
					right_range++;
				}
				rand_send(sockfd, ack_pkt, addr, loss_prob);
				break;
			case DATA:
				//qualunque sia il pacchetto ricevuto, invio ack
				cnt_inactive = 0;
				decrease_timeout(sockfd);
				ack_pkt->n_seq = rcv_pkt->n_seq;
				rand_send(sockfd, ack_pkt, addr, loss_prob);
				if(left_range < right_range) {
					if(rcv_pkt->n_seq >= left_range && rcv_pkt->n_seq <= right_range){
						//se è nel range
						memcpy(&(buff_pkts[rcv_pkt->n_seq % capacity]),
																										 rcv_pkt, sizeof(*rcv_pkt));
						if(rcv_pkt->n_seq == left_range) {
							//pkt più vecchio ancora atteso
							while(buff_pkts[left_range % capacity].n_seq != -1) {
								size_to_write = (file_size - total_written < pkt_size_data) ?
								 											file_size - total_written : pkt_size_data;
								memcpy(buff + written, buff_pkts[left_range % capacity].data,
									 																							 size_to_write);
								written =(written+size_to_write)%(WRITABLE_BUFF*pkt_size_data);
								if(written == 0){
									if(write_block(fd, buff, WRITABLE_BUFF * pkt_size_data) < 0){
										perror("write_block in sr_rcv_side");
										_exit(EXIT_FAILURE);
									}
									memset(buff, 0, WRITABLE_BUFF * pkt_size_data);
								} else if(size_to_write != pkt_size_data){
									if(write_block(fd, buff, written) < 0){
										perror("write_block in sr_rcv_side");
										_exit(EXIT_FAILURE);
									}
								}
								total_written += size_to_write;
								buff_pkts[left_range % capacity].n_seq = -1; //già inserito
                left_range = (left_range + 1) % capacity;
                right_range = (right_range + 1) % capacity;
							}
						} //else non è il pacchetto atteso, è stato salvato
					}//else il pacchetto non è nel nostro range, l'ack è già stato inviato
				} else {
          if(rcv_pkt->n_seq >= left_range || rcv_pkt->n_seq <= right_range){
						//se è nel range
						memcpy(&(buff_pkts[rcv_pkt->n_seq % capacity]), rcv_pkt,
						 																								  sizeof(*rcv_pkt));
						if(rcv_pkt->n_seq == left_range) {
							//pkt effettivamente atteso
							while(buff_pkts[left_range % capacity].n_seq != -1) {
								size_to_write = (file_size - total_written < pkt_size_data) ?
								 											file_size - total_written : pkt_size_data;
								memcpy(buff + written, buff_pkts[left_range % capacity].data,
									 																							 size_to_write);
								written =(written+size_to_write)%(WRITABLE_BUFF*pkt_size_data);
								if(written == 0){
									if(write_block(fd, buff, WRITABLE_BUFF * pkt_size_data) < 0){
										perror("write_block in sr_rcv_side");
										_exit(EXIT_FAILURE);
									}
									memset(buff, 0, WRITABLE_BUFF * pkt_size_data);
								} else if(size_to_write != pkt_size_data){
									if(write_block(fd, buff, written) < 0){
										perror("write_block in sr_rcv_side");
										_exit(EXIT_FAILURE);
									}
								}
								total_written += size_to_write;
								buff_pkts[left_range % capacity].n_seq = -1; //già inserito
                left_range = (left_range + 1) % capacity;
                right_range = (right_range + 1) % capacity;
							}
						} //else non è il pacchetto atteso, è stato salvato
					}//else il pacchetto non è nel nostro range, l'ack è già stato inviato
				}
				break;
			case ERROR:
				printf("%s\n", rcv_pkt->data);
				if(unlink(file_name) < 0){
					perror("unlink in sr_rcv_side");
					_exit(EXIT_FAILURE);
				}
				transferring = 0;
				break;
			case PUT:
				cnt_inactive = 0;
				decrease_timeout(sockfd);
				ack_pkt->n_seq = -1;
				rand_send(sockfd, ack_pkt, addr, loss_prob);
				break;
			case FIN:
				//pacchetto indicante la fine della trasmissione
				ack_pkt->type = FINACK;
				char again = 1;
				while (again) {
					//mando il finack
					rand_send(sockfd, ack_pkt, addr, loss_prob);

					for (tries = 5; tries > 0; --tries){
						// il receiver attende un eventuale fin ripetuto da parte del sender
						errno = 0;
						if(recvfrom(sockfd, rcv_pkt, sizeof(*rcv_pkt), 0,
						 						(struct sockaddr *)addr, &size) < 0 && errno != EAGAIN){
							perror("rcvfrom sr_rcv_side");
							_exit(EXIT_FAILURE);
						}
						if (errno == EAGAIN) {
							again = 0;
						} else if(rcv_pkt->type == FIN) {
							again = 1;
							break;
						}
					}
				}
				transferring = 0;
				gettimeofday(end, NULL);
				tot_time = (end->tv_sec - start->tv_sec) +
				 									 (float)labs(end->tv_usec - start->tv_usec) / 1000000;
				speed = (float)file_size / 1000 / tot_time;
				printf("Transfer of %s completed in [%fsec], speed: [%f Kbyte/s]\n\n",
				 																						file_name, tot_time, speed);
				break;
			default :
				//non è un pacchetto corretto, lo ignoro
				cnt_inactive = 0;
				continue;
		}
	}


	/*INIZIO PARTE PER TESTING SUI PARAMETRI*/
	/*
	int fdcsv;
	char *buffcsv;
	char tot_time_buff[30];
	char loss_prob_buff[30];
	char file_size_buff[30];
	char window_buff[20];
	if((fdcsv = open("./Testing/report.csv", O_WRONLY | O_CREAT, 0666)) < 0){
			perror("opening sr_rcv_side");
			_exit(EXIT_FAILURE);
	}
	if((lseek(fdcsv, 0, SEEK_END)) < 0) {
			perror("lseek in sr_rcv_side");
			_exit(EXIT_FAILURE);
	}
	if((buffcsv = calloc(1, 115)) == NULL) {
			perror("calloc sr_rcv_side");
			_exit(EXIT_FAILURE);
	}
	sprintf(tot_time_buff, "%f", tot_time);
	sprintf(loss_prob_buff, "%f", loss_prob);
	sprintf(file_size_buff, "%ld", file_size/1024);
	sprintf(window_buff, "%d", WINDOW);
	//riempio buff con x,y,z,w + \n
	strcat(buffcsv, "\n");
	strcat(buffcsv, file_size_buff);
	strcat(buffcsv, ",");
	strcat(buffcsv, window_buff);
	strcat(buffcsv, ",");
	strcat(buffcsv, tot_time_buff);
	strcat(buffcsv, ",");
	strcat(buffcsv, loss_prob_buff);

	if(write_block(fdcsv, buffcsv, strlen(buffcsv)) < 0){
			perror("write_block in sr_rcv_side");
			_exit(EXIT_FAILURE);
	}

	if(close(fdcsv) < 0){
			perror("closing sr_rcv_side");
			_exit(EXIT_FAILURE);
	}

	free(buffcsv);
	*/
	/*FINE PARTE PER TESTING SUI PARAMETRI*/
	free(buff);
	free(end);
	free(start);
	free(rcv_pkt);
	free(ack_pkt);
	if(close(fd) < 0){
		perror("close in sr_rcv_side");
		_exit(EXIT_FAILURE);
	}
}

void sr_snd_side(int sockfd, struct sockaddr_in *addr, char *file_name,
	 																								 int window, float loss_prob){
	int fd, cnt_inactive = 0, tries = 0, capacity = 2 * window + 1;
	cb_window *wb;
	t_pkt *ack_pkt, *fin_pkt;
	socklen_t size = sizeof(*addr);
	struct timeval *start, *end;
	if((start = calloc(1, sizeof(*start))) == NULL) {
		perror("calloc in sr_snd_side");
		_exit(EXIT_FAILURE);
	}
	if((end = calloc(1, sizeof(*end))) == NULL) {
		perror("calloc in sr_snd_side");
		_exit(EXIT_FAILURE);
	}

	//ALLOCAZIONE E APERTURA FILE
	if((wb = calloc(1, sizeof(*wb))) == NULL){
		perror("calloc in sr_snd_side");
		_exit(EXIT_FAILURE);
	}
	// WINDOW + 1 per evitare ambiguità nell'uguaglianza degli indici
	if((wb->buff_pkts = calloc(window + 1, sizeof(*ack_pkt))) == NULL) {
		perror("calloc in sr_snd_side");
		_exit(EXIT_FAILURE);
	}
	if((ack_pkt = calloc(1, sizeof(*ack_pkt))) == NULL) {
		perror("calloc in sr_snd_side");
		_exit(EXIT_FAILURE);
	}
	if((fd = open(file_name, O_RDONLY)) < 0){
		perror("open in sr_snd_side");
		_exit(EXIT_FAILURE);
	}

	// ROUTINE
	ssize_t n_read = 1;
	int n_snd, pkt_size_data = PKT_SIZE - (sizeof(short int) + sizeof(int));
	int seq = 0, expct_n_seq = 0;
	off_t file_size;
	float tot_time, speed;
	/*buff_ack è la struttura per il salvataggio degli ack
	 *vengono immagazzinati fino a 2window + 1 ack, tale valore è il risultato di
	 *studi per il riutilizzo dei numeri di sequenza
	 */
	short int *buff_ack;
	if((buff_ack = malloc((capacity) * sizeof(*buff_ack))) == NULL){
		perror("malloc in sr_snd_side");
		_exit(EXIT_FAILURE);
	}
	for(int i = 0; i < (capacity); ++i) {
		buff_ack[i] = -1;
	}
	gettimeofday(start, NULL);
	printf("Starting trasmission of %s\n", file_name);
	//Invio grandezza file
	file_size = get_filesize(fd);
	wb->buff_pkts[wb->snd].type = DATA_SIZE;
	snprintf(wb->buff_pkts[wb->snd].data, sizeof(off_t) + 5,"%ld", file_size);
	//fino a 999GB
	wb->buff_pkts[wb->snd].n_seq = seq;
	rand_send(sockfd, &wb->buff_pkts[wb->snd], addr, loss_prob);
	seq ++;
	wb->snd++;

	//per i controlli su quanti pkt attendere e inviare
	int acked = 0;
	int total_pkt;
	total_pkt = (int) ceil((double) file_size / (double) pkt_size_data) + 1;

	while(total_pkt != acked) {
		//fin quando il file non è terminato o siamo ancora in attesa di ack

		n_snd = (wb->snd + 1) % (window + 1);
		if(wb->expct != n_snd && n_read != 0) {
			// possiamo leggere da file e inviare sulla socket
			if((n_read = read_block(fd, wb->buff_pkts[wb->snd].data,
				 																							  pkt_size_data)) == -1) {
				perror("error in sr_snd_side");
				_exit(EXIT_FAILURE);
			}
			if(n_read != 0){
				wb->buff_pkts[wb->snd].type = DATA;
				wb->buff_pkts[wb->snd].n_seq = seq;
				buff_ack[seq] = -1; //in attesa di ack
				seq = (seq + 1) % (capacity); //n_seq tra [0, 2W +1]
				rand_send(sockfd, &wb->buff_pkts[wb->snd], addr, loss_prob);
				wb->snd = n_snd;
		  }
		} else {
			if(wb->expct != wb->snd){
				errno = 0;
				if(recvfrom(sockfd, ack_pkt, sizeof(*ack_pkt), 0,
				 							(struct sockaddr *)addr, &size) < 0 && errno != EAGAIN) {
					perror("recvfrom in sr_snd_side");
					_exit(EXIT_FAILURE);
				}
				if(errno == EAGAIN) {
					cnt_inactive++;
					if(cnt_inactive == MAX_INACTIVE_TRIES){
						printf("Inactive receiver\n");
						free(buff_ack);
						free(wb->buff_pkts);
						free(wb);
						free(end);
						free(start);
						free(ack_pkt);
						_exit(EXIT_FAILURE);
					}
					increase_timeout(sockfd);
					// scade il timeout, rimandiamo tutti i pacchetti non ackati
					for(int i = 0; i < window; i++){
						if(buff_ack[wb->buff_pkts[(wb->expct+i)%(window + 1)].n_seq] == -1){
							rand_send(sockfd, &wb->buff_pkts[(wb->expct + i)%(window + 1)],
							 																								 addr, loss_prob);
						}
					}
					continue;
				}
				if(ack_pkt->type == ACK) {
					cnt_inactive = 0;
					decrease_timeout(sockfd);
					expct_n_seq = wb->buff_pkts[wb->expct].n_seq;
					if(ack_pkt->n_seq == expct_n_seq){
						/* l'ack è quello del pacchetto più vecchio in attesa di ack,
						 *posso spostare la finestra
						 */
						do {
							acked++;
							wb->expct = (wb->expct + 1) % (window + 1);
							expct_n_seq = (expct_n_seq + 1) % (capacity);

							//aggiorno valori nel buff per evitare errori
							if(expct_n_seq - 1 < 0){
								buff_ack[capacity - 1] = -1;
							} else{
								buff_ack[expct_n_seq - 1] = -1;
							}
							buff_ack[(expct_n_seq + window - 1)%(capacity)] = -1;
							buff_ack[(expct_n_seq + window)%(capacity)] = -1;
						} while(buff_ack[expct_n_seq] != -1 && acked != total_pkt);
					} else {
						buff_ack[ack_pkt->n_seq] = 1; //ack di uno degli altri pacchetti
					}
				} else {
					//se non fosse ack allora ignoro
					continue;
				}
			} //fine if
		}//fine if-else
	} //fine while
	free(buff_ack);
	free(wb->buff_pkts);
	free(wb);
	if((fin_pkt = calloc(1, sizeof(*fin_pkt))) == NULL) {
		perror("calloc in sr_snd_side");
		_exit(EXIT_FAILURE);
	}
	//TERMINAZIONE TRASFERIMENTO
	fin_pkt->type = FIN;
	for(tries = 10; tries > 0; --tries){
		rand_send(sockfd, fin_pkt, addr, loss_prob);
		errno = 0;
		if(recvfrom(sockfd, ack_pkt, sizeof(*ack_pkt), 0,
		 									 (struct sockaddr *)addr, &size) < 0 && errno != EAGAIN) {
			perror("recvfrom in snd_side");
			_exit(EXIT_FAILURE);
		}
		if(errno == EAGAIN){
			continue;
		}
		if(ack_pkt->type == FINACK) {
			gettimeofday(end, NULL);
			tot_time = (end->tv_sec - start->tv_sec) +
													 (float)labs(end->tv_usec - start->tv_usec) / 1000000;
			speed = (float)file_size / 1000 / tot_time;
			printf("Transfer of file %s completed in [%fsec],speed:[%f Kbyte/s]\n\n",
			 																							file_name, tot_time, speed);
			break;
		}
	}
	if(tries == 0) {
		//non abbiamo ricevuto finack
		printf("End of transmission error.\n");
		printf("Not sure if the file has been saved.\n");
	}
	if(close(fd) < 0){
		perror("close in sr_snd_side");
		_exit(EXIT_FAILURE);
	}
	free(end);
	free(start);
	free(ack_pkt);
	free(fin_pkt);
}
