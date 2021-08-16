CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O3
LIB?= ./lib/
SEL?= ./selectiveRepeat/
FILEC = $(LIB)basic.h $(LIB)file_handler.c $(SEL)sr_transfer.c $(LIB)utility_client.c reliable_client.c -lm
FILES = $(LIB)basic.h $(LIB)file_handler.c $(SEL)sr_transfer.c $(LIB)utility_server.c reliable_server.c -lm

do:
	$(CC) $(CFLAGS) $(FILEC) -o client
	$(CC) $(CFLAGS) $(FILES) -o server

	@echo " "
	@echo "Compilato"
clean:
	rm client
	rm server

	@echo " "
	@echo "File eliminati!"
