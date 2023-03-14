CC=gcc

CFLAGS=-g

INCLUDES=-I./include

LIBINCLUDES=-lpthread

INC=$(INCLUDES) $(LIBINCLUDES)

SRC=./src/

build:
	$(CC) $(CFLAGS) $(INC) -o aubatch $(SRC)aubatch.c $(SRC)menu.c $(SRC)scheduler.c $(SRC)dispatcher.c $(SRC)job_queue.c $(SRC)job.c $(SRC)autest.c

sampleProgram:
	$(CC) $(CFLAGS) $(INC) -o sampleProgram $(SRC)sampleProgram.c

batch:
	$(CC) $(CFLAGS) $(INC) -o batch_job $(SRC)batch_job.c
