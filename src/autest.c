/*
 * COMP7500/7506
 * Project 3: AUbatch - A Batch Scheduling System
 * 
 * Joshua Boyd
 * Department of Computer Science and Software Engineering
 * Auburn University
 * Mar. 14, 2023. Version 1.1
 *  
 * AUbatch is a CPU scheduling emulator. This file defines the testing module. 
 *  
 * Compilation Instruction:
 *     Compile using the 'make' command*  
 *     
 * How to run aubatch_sample?
 *     1. You need to compile the batch_job file: ./src/batch_job.c
 *     2. The "batch_job" program (see ./src/batch_job.c) takes one argument
 *        from the commandline that is the run time.
 *     3. In aubtach: type 'run batch_job 5 10' to submit program "batch_job" as a job.
 */



#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include <scheduler.h>
#include <autest.h>
#include <aubatch.h>

Tester tester;

TestCase test_case;

pthread_mutex_t tester_condition_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t tester_schedule_condition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t tester_mutex = PTHREAD_MUTEX_INITIALIZER; // for accessing test_running variable from dispatcher

void* testingModule() {

    srand(time(NULL));

    while(quit_flag != 1) {
        pthread_mutex_lock(&tester_condition_mutex);
        if(tester.test_started <= 0) {
            pthread_cond_wait(&tester_schedule_condition, &tester_condition_mutex);
        }
        pthread_mutex_unlock(&tester_condition_mutex);
        
        if(tester.test_started >= 1) {
            pthread_mutex_lock(&scheduler_mutex);
            
            scheduler.policy = tester.test_case.policy;
            pthread_mutex_unlock(&scheduler_mutex);
 
            printf("Test starting...\n");

            int job_count;
            printf("Adding Jobs...\n");
            printf("Feel free to add additional jobs or view jobs with the 'list' command\n");
            for(job_count=0; job_count < tester.test_case.num_of_jobs; job_count++) {
                user_job.job_name = strdup(tester.test_case.benchmark);
                user_job.arg_list[0] = strdup(tester.test_case.benchmark);
            
                int cpu_time = (rand() % tester.test_case.max_cpu_time) + tester.test_case.min_cpu_time;
                user_job.est_run_time = cpu_time;
                char* cpu_time_str;
                sprintf(cpu_time_str, "%d", cpu_time);
                user_job.arg_list[1] = strdup(cpu_time_str);

                int priority = (rand() % tester.test_case.max_priority_level) + 1;
                user_job.priority = priority;

                user_job.unix_arrival_time = time(NULL);
                localtime_r(&user_job.unix_arrival_time, &user_job.arrival_time);
                user_job.is_running = 0;

                pthread_mutex_lock(&scheduler_mutex);
                scheduler.job_cache = user_job;
                pthread_mutex_unlock(&scheduler_mutex);

                total_num_of_jobs++; 

                pthread_mutex_lock(&scheduler_condition_mutex);
                pthread_cond_signal(&scheduler_queue_condition);
                pthread_mutex_unlock(&scheduler_condition_mutex);
            
                int sleep_time = tester.test_case.arrival_rate * 100;
                sleep(sleep_time);
            }  

            printf("Finished Adding Jobs\n");

            // wait until the test is done

            pthread_mutex_lock(&tester_condition_mutex);
            if(tester.test_started >= 1) {
                pthread_cond_wait(&tester_schedule_condition, &tester_condition_mutex);
            }
            pthread_mutex_unlock(&tester_condition_mutex);

            printf("Total number of job submitted: %i\n", total_num_of_jobs);
            printf("Average turnaround time: %0.2f seconds\n", average_turnaround_time);
            printf("Average CPU time: %0.2f seconds\n", average_cpu_time);
            printf("Average wait time: %0.2f seconds\n", average_wait_time);
            double throughput = 1 / average_turnaround_time;
            printf("Throughput: %0.2f No./second \n\n", throughput);

            // call init to reset all of the metric values
            init();

        }
    }
}


