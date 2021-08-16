#include "./lib/basic.h"

void sig_handler(int sig) {
	(void)sig;
	int status;
	pid_t pid;
	while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) > 0);
}

int main(int argc, char const *argv[]) {
	/* configurazione opzionale dei vari parametri */
	if (argc > 2){
		printf("Error! Usage:\n");
		printf("For configuration mode: ");
		printf("%s + -c || -config\n", argv[0]);
		printf("For user mode: ");
		printf("%s\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	// inizializzo le variabili per la connessione
	char  *server_ip;
	int conn_port, window, timeout;
	float loss_prob;

	/* 	CONFIGURATION MODE 	*/
	if (argc == 2){
		if ( !strcmp(argv[1],"-c") || !strcmp(argv[1],"-config")){
			printf("Entering configuration mode:\n");
			conn_port = port_config(); // Changing connection port
			server_ip = ip_config(); //Changing Server IP
		} else{
			printf("Error! Usage:\n");
			printf("For configuration mode: ");
			printf("%s + -c || -config\n", argv[0]);
			printf("For user mode: ");
			printf("%s\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	} else{
		conn_port = SERV_PORT;
		server_ip = SERVIP;
	}

	int sockfd;
  struct sockaddr_in *servaddr;
	int res;

	if (signal(SIGCHLD, sig_handler) == SIG_ERR) {
		perror("signal in main");
		exit(EXIT_FAILURE);
	}
	if((servaddr = calloc(1, sizeof(*servaddr))) == NULL) {
    perror("calloc in main");
    exit(EXIT_FAILURE);
  }
	servaddr->sin_family = AF_INET;
	servaddr->sin_port = htons(conn_port);
	//Il formato è già stato verificato, controllo solo l'errore
	res = inet_pton(AF_INET, server_ip, &(*servaddr).sin_addr);
	if( res < 0) {
			perror("pton in main");
			exit(EXIT_FAILURE);
	}

  // Creo socket di connessione verso il server
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
     perror("socket in main");
     exit(EXIT_FAILURE);
  }

	printf("-> Socket created, connecting to [ip = %s]\n",
	 																							inet_ntoa(servaddr->sin_addr));

	set_timeout(sockfd, TIMEOUT_PKT);
	srand(time(NULL)); // imposto seed

	//3 way handshake lato client
	if (reliable_connection(sockfd, servaddr, &window, &loss_prob, &timeout) < 0){
		fprintf(stderr, "Error during connection\n");
		exit(EXIT_FAILURE);
	}
	printf("\nConnection established\n");
	printf("***************************\n");
	printf("CONFIGURATION PARAMETERS\n");
	printf("Window = %d\n", window);
	printf("Loss probability = %f\n", loss_prob);
	if(timeout != 0){
		printf("Timeout = %d milli-sec\n", timeout / 1000);
	} else{
		printf("Timeout adattativo con valore di default = %d milli-sec\n",
		 																												TIMEOUT_PKT / 1000);
	}
	printf("***************************\n");
	if(timeout == 0){
		static_time = 0;
	} else{
		static_time = 1;
	}
  short int parsed_cmd, work = 1;

	printf("\nChoose what you want to do\n");
	printf("   [L]IST ; [G]ET ; [P]UT ; [Q]UIT ; [H]ELP  \n");

	while(work) {
		//Aspetto richieste dell'utente da stdin
    parsed_cmd = menu_cli();

		//Interpreto la richiesta inserita dall'utente
    switch (parsed_cmd) {
			case HELP:
				printf("\nInsert:\n");
				printf("[L]IST : to get a list of the available files in the server\n");
				printf("[G]ET : to download a file from the server\n");
				printf("[P]UT : to upload a file to the server\n");
				printf("[Q]UIT : to close the connection with server\n");
				break;
      case LIST:
        // richiesta di listing dei file sul server
				printf("\n-------- You chose LIST --------\n");
        client_list_work(sockfd, servaddr, window, loss_prob, timeout);
        break;
      case GET:
        // download di un file
				printf("\n-------- You chose GET --------\n");
        client_get_work(sockfd, servaddr, window, loss_prob, timeout);
        break;
      case PUT:
        // upload di un file
				printf("\n-------- You chose PUT --------\n");
        client_put_work(sockfd, servaddr, window, loss_prob, timeout);
        break;
      case QUIT:
        //chiusura della connessione e terminazione del processo
				printf("\n-------- You chose QUIT --------\n");
				client_quit_work(sockfd, servaddr, loss_prob);
				work = 0;
				break;
      default:
				printf("->PLEASE INSERT A CORRECT COMMAND\n");
    }
	}
	free(servaddr);
  exit(EXIT_SUCCESS);
}
