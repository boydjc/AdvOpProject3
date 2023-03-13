#pragma once

#include <job.h>
#include <pthread.h>
#include <aubatch.h>

typedef struct Scheduler {
    char* policy;
    int queue_head;
    int expected_wait_time;
    Job job_cache;
} Scheduler;

void* schedulerModule();

pthread_cond_t scheduler_queue_condition;
pthread_mutex_t scheduler_condition_mutex;
pthread_mutex_t scheduler_mutex;

extern Scheduler scheduler;
extern pthread_cond_t scheduler_queue_condition;
extern pthread_mutex_t scheduler_condition_mutex;
extern pthread_mutex_t scheduler_mutex;