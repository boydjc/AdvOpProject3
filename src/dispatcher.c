#include <stdio.h>
#include <stdlib.h>

#include <dispatcher.h>
#include <scheduler.h>
#include <aubatch.h>
#include <pthread.h>
#include <job_queue.h>
#include <sys/types.h>
#include <unistd.h>


pthread_cond_t dispatcher_queue_condition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t dispatcher_condition_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t dispatcher_mutex = PTHREAD_MUTEX_INITIALIZER; // for accessing dispatcher tail from main thread

Dispatcher dispatcher;

void* dispatcherModule(void* ptr) {
    //printf("Dispatcher is online and waiting...\n");

    while(quit_flag != 1) {
        pthread_mutex_lock(&dispatcher_condition_mutex);
        if(job_queue.queue_job_num <= 0) {
            pthread_cond_wait(&dispatcher_queue_condition, &dispatcher_condition_mutex);
        }
        pthread_mutex_unlock(&dispatcher_condition_mutex);
        
        int num_of_jobs = 0;
        pthread_mutex_lock(&queue_mutex);
        num_of_jobs = job_queue.queue_job_num;
        pthread_mutex_unlock(&queue_mutex);

        if(num_of_jobs != 0 && num_of_jobs < JOB_QUEUE_MAX_SIZE) {
            pthread_mutex_lock(&queue_mutex);
            //printf("Dispatcher detected job in queue\n");
            Job job = job_queue.queue[dispatcher.queue_tail];
            job_queue.queue[dispatcher.queue_tail].is_running = 1;
            pthread_mutex_unlock(&queue_mutex);

            pthread_mutex_lock(&dispatcher_mutex);
            //printf("\tDispatcher tail value before grabbing job: %i\n", dispatcher.queue_tail);
            if(dispatcher.queue_tail >= JOB_QUEUE_MAX_SIZE-1) {
                dispatcher.queue_tail = 0;
            } else {
                dispatcher.queue_tail++;
            }
            //printf("\tDispatcher tail value after grabbing job: %i\n", dispatcher.queue_tail);
            pthread_mutex_unlock(&dispatcher_mutex);

            if(job.job_name) {
                pid_t pid;
                time_t start_time = time(NULL);
                pid = fork();
 
                switch(pid) {
                    case -1:
                        perror("fork");
                        break;
                    case 0:
                        execv(job.job_name, job.arg_list);
                        perror("execv");
                        //printf("\tJob was: %s\n", job.job_name);
                        exit(EXIT_FAILURE);
                        break;
                    default:
                        pid = wait(NULL);
                        pthread_mutex_lock(&scheduler_mutex);
                        scheduler.expected_wait_time -= job.est_run_time;
                        pthread_mutex_unlock(&scheduler_mutex);
                        total_completed_jobs++;

                        time_t finish_time = time(NULL);
                        
                        double turnaround_time = difftime(finish_time, job.unix_arrival_time);

                        //printf("Turnaround Time: %0.2f\n", turnaround_time);
                                                
                        total_turnaround_time += turnaround_time;
                        average_turnaround_time = total_turnaround_time / total_completed_jobs;

                        //printf("Average Turnaround Time: %0.2f\n", average_turnaround_time);

                        double cpu_time = difftime(finish_time, start_time);
                        //printf("CPU Time: %0.2f\n", cpu_time);                        

                        total_cpu_time += cpu_time;
                        average_cpu_time = total_cpu_time / total_completed_jobs;

                        //printf("Average CPU Time: %0.2f\n", average_cpu_time);


                        double wait_time = difftime(turnaround_time, cpu_time);

                        //printf("Wait Time: %0.2f\n", wait_time);

                        total_wait_time += wait_time;
                        average_wait_time = total_wait_time / total_completed_jobs;

                        //printf("Average Wait Time: %0.2f\n", average_wait_time);

                        double response_time = difftime(start_time, job.unix_arrival_time);

                        total_response_time += response_time;
                        average_response_time = total_response_time / total_completed_jobs;

                        //printf("Response time: %0.2f\n", response_time);
                        //printf("Average Response Time: %0.2f\n\n", average_response_time);


                        pthread_mutex_lock(&queue_mutex);
                        job_queue.queue_job_num--;
                        pthread_mutex_unlock(&queue_mutex);

                        break;
                }

            }
        }           
    }

}