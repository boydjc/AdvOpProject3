CC=gcc

CFLAGS=-g

INCLUDES=-I./include

LIBINCLUDES=-lpthread

INC=$(INCLUDES) $(LIBINCLUDES)

SRC=./src/

build:
	$(CC) $(CFLAGS) $(INC) -o aubatch $(SRC)aubatch.c $(SRC)menu.c

sampleProgram:
	$(CC) $(CFLAGS) $(INC) -o sampleProgram $(SRC)sampleProgram.c

batch:
	$(CC) $(CFLAGS) $(INC) -o batch_job $(SRC)batch_job.c
