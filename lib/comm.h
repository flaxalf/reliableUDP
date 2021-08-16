// parametri di default

#define SERV_PORT	5193
#define PKT_SIZE 1500
#define MAX_INPUT_LINE 128

//tempo di attesa in secondi
#define MAX_WAIT_TIME 300
#define CLIENT_FOLDER "./clientFiles/"
#define SERVER_FOLDER "./serverFiles/"
#define TEMP_FOLDER "./tempFiles/"
#define SERVIP "127.0.0.1"
#define LOSS_PROB 0.20
#define WINDOW 20

//timeout in microsecondi
#define TIMEOUT_PKT 5000

//1 sec- 1 micro
#define MAX_TIMEOUT 999999
#define MIN_TIMEOUT 1000
#define TIME_UNIT 500
#define MAX_INACTIVE_TRIES 25

// 2^16 - 1 = 65'535 numero di porta massimo
#define MAX_PORT 65535
#define MAX_FILE_LIST 100
#define MAX_NAMEFILE_LEN 127
#define WRITABLE_BUFF 1000

#define STR_LIST "LIST"
#define CHR_LIST "L"
#define STR_GET "GET"
#define CHR_GET "G"
#define STR_PUT "PUT"
#define CHR_PUT "P"
#define STR_QUIT "QUIT"
#define CHR_QUIT "Q"
#define STR_HELP "HELP"
#define CHR_HELP "H"

#define LIST 0
#define GET 1
#define PUT 2
#define QUIT 3
#define ACK 4
#define DATA 5
#define SYN 6
#define SYNACK 7
#define ERROR 8
#define FIN 9
#define FINACK 10
#define DATA_SIZE 11
#define HELP 12

typedef struct pkt {
  short int type;
  int n_seq;
  char data[PKT_SIZE - (sizeof(short int) + sizeof(int))];
} t_pkt;

typedef struct circular_buffer {
  int expct;
  int snd;
  t_pkt *buff_pkts;
} cb_window;


#ifndef _BASIC_H_
  extern char static_time;
#else
  char static_time;
#endif
