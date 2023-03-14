#include <stdio.h>
#include <stdlib.h>

#include <dispatcher.h>
#include <scheduler.h>
#include <autest.h>
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

    int num_of_jobs = 0;

    while(quit_flag != 1 || num_of_jobs >= 1) {
        pthread_mutex_lock(&dispatcher_condition_mutex);
        if(job_queue.queue_job_num <= 0 && quit_flag <= 0) {
            pthread_cond_wait(&dispatcher_queue_condition, &dispatcher_condition_mutex);
        }
        pthread_mutex_unlock(&dispatcher_condition_mutex);

        pthread_mutex_lock(&queue_mutex);
        num_of_jobs = job_queue.queue_job_num;
        pthread_mutex_unlock(&queue_mutex);

        if(num_of_jobs != 0 && num_of_jobs < JOB_QUEUE_MAX_SIZE) {
            pthread_mutex_lock(&queue_mutex);
            Job job = job_queue.queue[dispatcher.queue_tail];
            job_queue.queue[dispatcher.queue_tail].is_running = 1;
            pthread_mutex_unlock(&queue_mutex);

            pthread_mutex_lock(&dispatcher_mutex);
            if(dispatcher.queue_tail >= JOB_QUEUE_MAX_SIZE-1) {
                dispatcher.queue_tail = 0;
            } else {
                dispatcher.queue_tail++;
            }
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

                                                
                        total_turnaround_time += turnaround_time;
                        average_turnaround_time = total_turnaround_time / total_completed_jobs;


                        double cpu_time = difftime(finish_time, start_time);

                        total_cpu_time += cpu_time;
                        average_cpu_time = total_cpu_time / total_completed_jobs;

                        double wait_time = difftime(turnaround_time, cpu_time);

                        total_wait_time += wait_time;
                        average_wait_time = total_wait_time / total_completed_jobs;

                        pthread_mutex_lock(&queue_mutex);
                        job_queue.queue_job_num--;
                        pthread_mutex_lock(&tester_mutex);
                        if(job_queue.queue_job_num <= 0 && tester.test_started >= 1) {
                            tester.test_started = 0;
                            pthread_mutex_lock(&tester_condition_mutex);
                            pthread_cond_signal(&tester_schedule_condition);
                            pthread_mutex_unlock(&tester_condition_mutex);
                        }
                        pthread_mutex_unlock(&tester_mutex);
                        pthread_mutex_unlock(&queue_mutex);

                        break;
                }

            }
        }           
    }

    if(quit_flag >= 1 && total_num_of_jobs >= 1) {
        printf("Total number of job submitted: %i\n", total_num_of_jobs);
        printf("Average turnaround time: %0.2f seconds\n", average_turnaround_time);
        printf("Average CPU time: %0.2f seconds\n", average_cpu_time);
        printf("Average wait time: %0.2f seconds\n", average_wait_time);

        double throughput = 1 / average_turnaround_time;

        printf("Throughput: %0.2f No./second \n\n", throughput);
    }

}
