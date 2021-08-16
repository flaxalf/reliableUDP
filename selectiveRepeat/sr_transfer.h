int get_timeout(int sockfd);
void set_timeout(int sockfd, int timeout);
void set_timeout_sec(int sockfd, int timeout);
void decrease_timeout(int sockfd);
void increase_timeout(int sockfd);
int create_socket(int timeout);

void rand_send(int sockfd, t_pkt *pkt, struct sockaddr_in *addr, float loss_prob);

void sr_rcv_side(int sockfd, struct sockaddr_in *addr, char *file_name, int window, float loss_prob);
void sr_snd_side(int sockfd, struct sockaddr_in *addr, char *file_name, int window, float loss_prob);
