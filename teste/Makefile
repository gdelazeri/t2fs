CC=gcc
LIB_DIR=../lib

all: teste teste_2

teste: teste.c $(LIB_DIR)/libt2fs.a
	$(CC) -o teste teste.c -L$(LIB_DIR) -lt2fs -Wall

teste_2: teste_2.c $(LIB_DIR)/libt2fs.a
	$(CC) -o teste_2 teste_2.c -L$(LIB_DIR) -lt2fs -Wall



clean:
	rm -rf teste *.o *~
