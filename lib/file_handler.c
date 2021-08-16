#include "basic.h"

ssize_t read_block(int fd, void *buf, size_t n) {
  //legge n byte dal file
  size_t  nleft;
  ssize_t nread;
  char *ptr;

  ptr = buf;
  nleft = n;
  while (nleft > 0) {
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR)
        nread = 0;
      else
        return(-1);
    }
    else if (nread == 0)
      break;	/* EOF */

    nleft -= nread;
    ptr += nread;
  }
  return(n-nleft);	// restituisce 0 solo se non ha letto niente, altrimenti > 0
}
ssize_t write_block(int fd, const void *buf, size_t n) {
  //scrive n byte nel file
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;

  ptr = buf;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
       if ((nwritten < 0) && (errno == EINTR))
        nwritten = 0;
       else
        return(-1);	//errore
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return(n-nleft);// restituisce numero di byte scritti o -1 in caso di errore
}

char* create_path(char *folder, char *file_name) {
  //restituisce la path del file
  char *path;

  if((path = calloc(1, strlen(folder) + strlen(file_name) + 3 + 1)) == NULL){
      //+3 per eventuali indici (1), (2), ... mentre + 1 per \0
		perror("calloc path in list");
		_exit(EXIT_FAILURE);
	}
	strcpy(path, folder);
	strcat(path, file_name);

  return path;
}

int check_file(char *file_name) {
  //controlla se il file esiste
	int exist, fd;
	errno = 0;

	fd = open(file_name, O_RDONLY);
	if(fd < 0 && errno != ENOENT ){
		perror("open in check file");
		_exit(EXIT_FAILURE);
	}
	if(errno == ENOENT){
		//il file non esiste
		exist = -1;
	} else {
		if(close(fd) < 0) {
			perror("close in check_file");
			_exit(EXIT_FAILURE);
		}
		exist = 0;
	}
	return exist;
}
int files_from_folder(char *list_files[MAX_FILE_LIST]) {
  /* apre la cartella e prende tutti i nomi dei file presenti in essa,
   * inserendoli in un buffer e ritornando il numero di file presenti
   */
  int i = 0;
  DIR *dp;
  struct dirent *ep;
  for(; i < MAX_FILE_LIST; ++i) {
    if ((list_files[i] = malloc(MAX_NAMEFILE_LEN * sizeof(char))) == NULL) {
      perror("malloc list_files");
      exit(EXIT_FAILURE);
    }
  }

  dp = opendir(SERVER_FOLDER);
  if(dp != NULL){
    i = 0;
    while((ep = readdir(dp))) {
      if(strncmp(ep->d_name, ".", 1) != 0 && strncmp(ep->d_name, "..", 2) != 0){
        strncpy(list_files[i], ep->d_name, MAX_NAMEFILE_LEN);
        ++i;
      }
    }
    closedir(dp);
  }else{
    perror ("Couldn't open the directory");
  }
  return i;
}

off_t get_filesize(int fd) {
  //restituisce la dimensione del file
  off_t size;
  if((size = lseek(fd, 0, SEEK_END)) < 0) {
    perror("lseek in get_filesize");
    _exit(EXIT_FAILURE);
  }
  if((lseek(fd, 0, SEEK_SET)) < 0) {
    perror("lseek in get_filesize");
    _exit(EXIT_FAILURE);
  }
  return size;
}
