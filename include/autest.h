#pragma once

#include <pthread.h>

typedef struct TestCase {
    char* benchmark;
    char* policy;
    int num_of_jobs;
    float arrival_rate;
    int max_priority_level;
    int min_cpu_time;
    int max_cpu_time;
} TestCase;

typedef struct Tester {
    int test_started;
    TestCase test_case;
} Tester;

void* testingModule();

pthread_mutex_t tester_condition_mutex;
pthread_mutex_t tester_mutex;
pthread_cond_t tester_schedule_condition;

extern TestCase test_case;
extern Tester tester;
extern pthread_mutex_t tester_condition_mutex;
extern pthread_cond_t tester_schedule_condition;
extern pthread_mutex_t tester_mutex;
