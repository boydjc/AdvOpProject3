#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <scheduler.h>
#include <autest.h>
#include <aubatch.h>

Tester tester;

TestCase test_case;

pthread_mutex_t tester_condition_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t tester_schedule_condition = PTHREAD_COND_INITIALIZER;

void* testingModule() {
    printf("Tester is online and waiting...\n");

    srand(time(NULL));

    while(quit_flag != 1) {
        pthread_mutex_lock(&tester_condition_mutex);
        if(tester.test_started <= 0) {
            pthread_cond_wait(&tester_schedule_condition, &tester_condition_mutex);
        }
        pthread_mutex_unlock(&tester_condition_mutex);

        pthread_mutex_lock(&scheduler_mutex);
        scheduler.policy = tester.test_case.policy;
        pthread_mutex_unlock(&scheduler_mutex);

        printf("Test starting...\n");

        int job_count;
        for(job_count=0; job_count < tester.test_case.num_of_jobs; job_count++) {
            Job job;
            job.job_name = strdup(tester.test_case.benchmark);
            job.arg_list[0] = strdup(tester.test_case.benchmark);
            
            int cpu_time = (rand() % tester.test_case.max_cpu_time) + tester.test_case.min_cpu_time;
            char* cpu_time_str;
            sprintf(cpu_time_str, "%d", cpu_time);
            job.arg_list[1] = strdup(cpu_time_str);

            int priority = (rand() % tester.test_case.max_priority_level) + 1;
            job.priority = priority;

            job.unix_arrival_time = time(NULL);
            localtime_r(&job.unix_arrival_time, &job.arrival_time);
            job.is_running = 0;

            printf("Adding job...\n");
            pthread_mutex_lock(&scheduler_mutex);
            scheduler.job_cache = job;
            pthread_mutex_unlock(&scheduler_mutex);

            total_num_of_jobs++; 

            pthread_mutex_lock(&scheduler_condition_mutex);
            pthread_cond_signal(&scheduler_queue_condition);
            pthread_mutex_unlock(&scheduler_condition_mutex);
            
            printf("Sleeping for %f seconds\n", tester.test_case.arrival_rate * 100);
            //sleep(tester.test_case.arrival_rate * 100);
            sleep(5);
        }  

        tester.test_started = 0;
    }
}


