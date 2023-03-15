/*
 * COMP7500/7506
 * Project 3: AUbatch - A Batch Scheduling System
 *  
 * Joshua Boyd
 * Department of Computer Science and Software Engineering
 * Auburn University
 * Mar. 14, 2023. Version 1.1
 *  
 * AUbatch is a CPU scheduling emulator. This file defines the scheduler module. 
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

#include <scheduler.h>
#include <dispatcher.h>
#include <job.h>
#include <job_queue.h>
#include <pthread.h>
#include <aubatch.h>

pthread_cond_t scheduler_queue_condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t scheduler_condition_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t scheduler_mutex = PTHREAD_MUTEX_INITIALIZER; // for accessing expected wait time from dispatcher thread

Scheduler scheduler;

void* schedulerModule(void* ptr) {

    while(quit_flag != 1) {
        pthread_mutex_lock(&scheduler_condition_mutex);
        pthread_cond_wait(&scheduler_queue_condition, &scheduler_condition_mutex);  
        pthread_mutex_unlock(&scheduler_condition_mutex);
        
        int num_of_jobs = 0;
        pthread_mutex_lock(&queue_mutex);
        num_of_jobs = job_queue.queue_job_num;
        pthread_mutex_unlock(&queue_mutex);

        if(quit_flag != 1) {
            pthread_mutex_lock(&queue_mutex);
            job_queue.queue[scheduler.queue_head] = scheduler.job_cache;
            job_queue.queue_job_num++;
            pthread_mutex_unlock(&queue_mutex);

            if(scheduler.queue_head >= JOB_QUEUE_MAX_SIZE-1) {
                scheduler.queue_head = 0;
            }else {
                scheduler.queue_head++;
            }

            
            reallocateJobQueue();

            // signal to the dispatcher
            pthread_mutex_lock(&dispatcher_condition_mutex);
            pthread_cond_signal(&dispatcher_queue_condition);
            pthread_mutex_unlock(&dispatcher_condition_mutex);
        }         
    }
 
}
