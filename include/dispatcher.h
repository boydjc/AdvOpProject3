#pragma once

#include <pthread.h>
#include <aubatch.h>

typedef struct Dispatcher {
    int queue_tail;
} Dispatcher;

void* dispatcherModule();

pthread_cond_t dispatcher_queue_condition;
pthread_mutex_t dispatcher_condition_mutex;
pthread_mutex_t dispatcher_mutex;

extern Dispatcher dispatcher;
extern pthread_cond_t dispatcher_queue_condition;
extern pthread_mutex_t dispatcher_condition_mutex;
extern pthread_mutex_t dispatcher_mutex;