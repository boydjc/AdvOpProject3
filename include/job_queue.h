#pragma once

#include <job.h>
#include <pthread.h>

#define JOB_QUEUE_MAX_SIZE 100

typedef struct JobQueue {
    Job queue[JOB_QUEUE_MAX_SIZE];
    int queue_job_num;
} JobQueue;

pthread_mutex_t queue_mutex;

extern pthread_mutex_t queue_mutex;
extern JobQueue job_queue;