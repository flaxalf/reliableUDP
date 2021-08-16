#include "./lib/basic.h"

void sig_handler(int sig) {
	(void)sig;
	int status;
	pid_t pid;
	while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) > 0)
		printf ("Child %d terminated with %d\n", pid, status);
}

int main(int argc, char **argv) {
  if (argc > 2){
		printf("Error! Usage:\n");
		printf("For configuration mode: ");
		printf("%s + -c || -config\n", argv[0]);
		printf("For server mode: ");
		printf("%s\n", argv[0]);
		exit(EXIT_FAILURE);
	}
  int conn_port, window, timeout;
	float loss_prob = 0;

  if (argc == 2){
    /* CONFIGURATION MODE */
    if ( !strcmp(argv[1],"-c") || !strcmp(argv[1],"-config")){
			printf("Entering configuration mode:\n");
      conn_port = port_config();
      loss_prob = prob_config();
      window = window_config();
      timeout = timeout_config();
    } else{
      printf("Error! Usage:\n");
  		printf("For configuration mode: ");
  		printf("%s + -c || -config\n", argv[0]);
  		printf("For server mode: ");
  		printf("%s\n", argv[0]);
  		exit(EXIT_FAILURE);
    }
  } else{
    conn_port = SERV_PORT;
    loss_prob = LOSS_PROB;
    window = WINDOW;
    timeout = TIMEOUT_PKT;
  }
	if(timeout == 0){
		static_time = 0;
	} else{
		static_time = 1;
	}
	/* GESTORE SEGNALE & FASE CREAZIONE SOCKET UDP */
  int sockfd, new_sockfd, cmd_frm_rcv;
  struct sockaddr_in *servaddr, *cliaddr;
	pid_t pid;
  /* Utilizzato per recuperare risorse dei figli terminati, raccogliendo il
	 * segnale SIGCHLD da loro emanato
	 */
  if (signal(SIGCHLD, sig_handler) == SIG_ERR) {
    perror("signal in main");
    exit(EXIT_FAILURE);
  }
  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket in main");
    exit(EXIT_FAILURE);
  }
  if((servaddr = calloc(1, sizeof(*servaddr))) == NULL) {
    perror("calloc in main");
    exit(EXIT_FAILURE);
  }

  servaddr->sin_family = AF_INET;
  servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr->sin_port = htons(conn_port);
  if(bind(sockfd, (struct sockaddr *)servaddr, sizeof(*servaddr)) < 0) {
    perror("bind in main");
    exit(EXIT_FAILURE);
  }
  set_timeout(sockfd, timeout);
  printf("-> Listening socket created [port = %d]\n", ntohs(servaddr->sin_port));
	free(servaddr);
  /* Socket creata */

  /* Struttura per informazioni sul futuro client in arrivo */
  if((cliaddr = calloc(1, sizeof(*cliaddr))) == NULL) {
     perror("calloc in main");
     exit(EXIT_FAILURE);
  }

  /* Fase in cui il processo principale si mette in ascolto di eventuali
   * richieste dei client, affidando il lavoro ai figli
   */
  socklen_t len = sizeof(cliaddr);
  /* Ciclo per l'ascolto e gestione richieste */
  for(;;) {
    //ritorna 0 se è il padre o new_sockfd per il figlio
    if((new_sockfd = reliable_accept(sockfd, cliaddr, &len, window, loss_prob, timeout)) == 0){
      continue;
    }
		/* FIGLIO */
    t_pkt *rcv_pkt;
    if((rcv_pkt = (t_pkt *) calloc(1, sizeof(*rcv_pkt))) == NULL) {
      perror("calloc in main");
      exit(EXIT_FAILURE);
		}
    /* Figlio si mette in attesa di ricezione comandi da un client, per poi
     * identificare il tipo di richiesta.
     */
  waitcmd :
		errno = 0;
		//Timeout di attesa comando, se superato il client risulta inattivo
    set_timeout_sec(new_sockfd, MAX_WAIT_TIME);

    if(recvfrom(new_sockfd, (void *)rcv_pkt, sizeof(*rcv_pkt), 0, (struct sockaddr *)cliaddr, &len) < 0 && errno != EAGAIN) {
			if(errno == EINTR) {
				/* Caso in cui siamo stati interrotti in quanto un figlio ha teterminato
				* il suo ciclo di vita, rientreremo in rcvfrom
				*/
				goto waitcmd;
			}
			perror("recvfrom in main");
      _exit(EXIT_FAILURE);
    }
    if(errno == EAGAIN){
      //client inattivo
      printf("Inactive client -> Closing connection [pid_close = %d]\n",
			 																																getpid());
			free(rcv_pkt);
			free(cliaddr);
			if(close(new_sockfd) < 0){
				perror("close in main");
				_exit(EXIT_FAILURE);
			}
      _exit(EXIT_SUCCESS);
    }
    // timeout torna ad essere quello configurato per gli scambi di pacchetti
    set_timeout(new_sockfd, timeout);

    cmd_frm_rcv = rcv_pkt->type;

    switch (cmd_frm_rcv) {
      case LIST:
				if ((pid = fork()) < 0) {
					perror("fork in main");
					exit(EXIT_FAILURE);
				}
				if(pid != 0){
					goto waitcmd;
				}
				new_sockfd = create_socket(timeout);
        server_list_work(new_sockfd, cliaddr, window, loss_prob);
        printf("Command LIST completed [figlio = %d]\n", getpid());
				if (close(new_sockfd) < 0) {
			    perror("close in main");
			    exit(EXIT_FAILURE);
			  }
        _exit(EXIT_SUCCESS);
      case GET:
				if ((pid = fork()) < 0) {
					perror("fork in main");
					exit(EXIT_FAILURE);
				}
				if(pid != 0){
					goto waitcmd;
				}
				new_sockfd = create_socket(timeout);
        server_get_work(new_sockfd, cliaddr, rcv_pkt, window, loss_prob);
        printf("Command GET completed [figlio = %d]\n", getpid());
				if (close(new_sockfd) < 0) {
			    perror("close in main");
			    exit(EXIT_FAILURE);
			  }
        _exit(EXIT_SUCCESS);
      case PUT:
				if ((pid = fork()) < 0) {
					perror("fork in main");
					exit(EXIT_FAILURE);
				}
				if(pid != 0){
					goto waitcmd;
				}
				new_sockfd = create_socket(timeout);
        server_put_work(new_sockfd, cliaddr, rcv_pkt, window, loss_prob);
        printf("Command PUT completed [figlio = %d]\n", getpid());
				if (close(new_sockfd) < 0) {
			    perror("close in main");
			    exit(EXIT_FAILURE);
			  }
				_exit(EXIT_SUCCESS);
      case QUIT:
        // chiusura connessione con il rispettivo client
        free(rcv_pkt);
        server_quit_work(new_sockfd, cliaddr);
        _exit(EXIT_FAILURE);
      default:
        goto waitcmd;
     } // fine switch
   } // fine for

   free(servaddr);
	 free(cliaddr);
   return EXIT_SUCCESS;
}
