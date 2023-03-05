CC=gcc

CFLAGS=-g

INCLUDES=-I./include

LIBINCLUDES=-lpthread

INC=$(INCLUDES) $(LIBINCLUDES)

SRC=./src/

OUT=-o ./bin/

build:
	$(CC) $(CFLAGS) $(INC) $(OUT)aubatch $(SRC)aubatch.c $(SRC)menu.c

sampleProgram:
	$(CC) $(CFLAGS) $(INC) $(OUT)sampleProgram $(SRC)sampleProgram.c

batch:
	$(CC) $(CFLAGS) $(INC) $(OUT)batch_job $(SRC)batch_job.c
