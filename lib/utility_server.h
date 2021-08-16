int port_config();
float prob_config();
int window_config();
int timeout_config();

int reliable_accept(int sockfd, struct sockaddr_in *cliaddr, socklen_t *len,
                    int window, float loss_prob, int timeout);

void server_list_work(int sockfd, struct sockaddr_in *cliaddr,
                      int window, float loss_prob);
void server_get_work(int sockfd, struct sockaddr_in *cliaddr,
                     t_pkt *pkt, int window, float loss_prob);
void server_put_work(int sockfd, struct sockaddr_in *cliaddr, t_pkt *pkt,
                     int window, float loss_prob);
void server_quit_work(int sockfd, struct sockaddr_in *cliaddr);
