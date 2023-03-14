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
