int port_config();
char* ip_config();

int reliable_connection(int sockfd, struct sockaddr_in *servaddr, int *window,
                        float *loss_prob, int *timeout);

int menu_cli();

void client_list_work(int sockfd, struct sockaddr_in *servaddr,
                      int window, float loss_prob, int timeout);
void client_get_work(int sockfd, struct sockaddr_in *servaddr,
                      int window, float loss_prob, int timeout);
void client_put_work(int sockfd, struct sockaddr_in *servaddr,
                      int window, float loss_prob, int timeout);
void client_quit_work(int sockfd, struct sockaddr_in *servaddr,float loss_prob);
