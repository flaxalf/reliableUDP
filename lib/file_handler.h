ssize_t read_block(int fd, void *buf, size_t n);
ssize_t write_block(int fd, const void *buf, size_t n);

char* create_path(char *folder, char *file_name);

int check_file(char *file_name);
int files_from_folder(char *list_files[MAX_FILE_LIST]);

off_t get_filesize(int fd);
