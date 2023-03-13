#pragma once

#include <time.h>

typedef struct Job {
    char* job_name;
    int est_run_time;
    int priority;
    time_t unix_arrival_time;
    struct tm arrival_time;
    int is_running;
    char* arg_list[20];
} Job;

extern Job user_job;