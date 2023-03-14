#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <scheduler.h>
#include <dispatcher.h>
#include <job_queue.h>
#include <menu.h>
#include <autest.h>

#define MAX_JOB_ARGS 20

/**********************
 *                    *
 *    Main Function   *
 *                    *
 **********************/

int main() {

    char* user_command;
    size_t user_command_size = 64; 
    user_command = (char*) malloc(user_command_size * sizeof(char));

    if(user_command == NULL) {
        fprintf(stderr, "Unable to malloc userCommand buffer\n");
        return 1;
    }

    init();

    pthread_t scheduler_thread, dispatcher_thread, tester_thread;

    int iret1 = pthread_create(&scheduler_thread, NULL, schedulerModule, NULL);
    int iret2 = pthread_create(&dispatcher_thread, NULL, dispatcherModule, NULL);
    int iret3 = pthread_create(&tester_thread, NULL, testingModule, NULL);

    displayGreeting();
    while(quit_flag == 0) {
        printf(">");
        getline(&user_command, &user_command_size, stdin);
        parseUserCommand(user_command);
    }

    pthread_join(scheduler_thread, NULL);
    pthread_join(dispatcher_thread, NULL);
    pthread_join(tester_thread, NULL);

    free(user_command);
    user_command = NULL;

    return 0;
}

/*****************************
 *                           *
 *    Function Definitions   *
 *                           *
 *****************************/

/*
 * This function gets things ready by setting a policy for the scheduler,
 * initializing the number of jobs in the queue, etc. */
void init() {

    quit_flag = 0;
    total_num_of_jobs = 0;
    total_turnaround_time = 0;
    average_turnaround_time = 0;
    total_cpu_time = 0;
    average_cpu_time = 0;
    total_wait_time = 0;
    average_wait_time = 0;
    total_completed_jobs = 0;

    scheduler.policy = "FCFS";
    scheduler.queue_head = 0;
    scheduler.expected_wait_time = 0;
 
    dispatcher.queue_tail = 0;
    
    job_queue.queue_job_num = 0;      
    
}


void parseUserCommand(char* user_command) {

    // tokenize the command and start taking action based on the tokens 
    char* command = strtok(user_command, " "); 

    char* cleaned_command = cleanCommand(command);    

    if(strcmp(cleaned_command, "quit") == 0) {
        quit_flag = 1;
        // after setting the quit flag, signal to both threads one more time in case they are waiting
        pthread_mutex_lock(&scheduler_condition_mutex);
        pthread_cond_signal(&scheduler_queue_condition);
        pthread_mutex_unlock(&scheduler_condition_mutex);

        pthread_mutex_lock(&queue_mutex);
        if(job_queue.queue_job_num > 0) {
            printf("Finishing remaining %i jobs...\n", job_queue.queue_job_num);
        }
        pthread_mutex_unlock(&queue_mutex);

        pthread_mutex_lock(&dispatcher_condition_mutex);
        pthread_cond_signal(&dispatcher_queue_condition);
        pthread_mutex_unlock(&dispatcher_condition_mutex);

        pthread_mutex_lock(&tester_condition_mutex);
        pthread_cond_signal(&tester_schedule_condition);
        pthread_mutex_unlock(&tester_condition_mutex);
       
    } else if(strcmp(cleaned_command, "help") == 0) {
        command = strtok(NULL, " ");
        if(command == NULL) {
            displayHelp();
        } else {
            cleaned_command = cleanCommand(command);
            if(strcmp(cleaned_command, "-run") == 0){
                displayRunHelp();
            } else if(strcmp(cleaned_command, "-list") == 0) {
                displayListHelp();
            } else if(strcmp(cleaned_command, "-fcfs") == 0) {
                displayFcfsHelp();
            } else if(strcmp(cleaned_command, "-sjf") == 0) {
                displaySjfHelp();
            } else if(strcmp(cleaned_command, "-priority") == 0) {
                displayPriorityHelp();
            } else if(strcmp(cleaned_command, "-test") == 0) {
                displayTestHelp();
            } else if(strcmp(cleaned_command, "-quit") == 0) {
                displayQuitHelp();
            } else {
                fprintf(stderr, "Error: Command not found. Make sure you are passing the '-' char with your command you need help with.\n");
                fprintf(stderr, "Example: help -test\n\n");
            }
        }

    } else if(strcmp(cleaned_command, "run") == 0) {
        command = strtok(NULL, " ");
        if(command == NULL) {
            fprintf(stderr, "ERROR: You must specify a file to run.\n\n");
            displayRunHelp();
        } else {
            cleaned_command = cleanCommand(command);
            user_job.job_name = strdup(cleaned_command);
            user_job.arg_list[0] = strdup(cleaned_command);
            
            command = strtok(NULL, " ");
            if(command == NULL) {
                fprintf(stderr, "ERROR: You must specify an estimated runtime length for the given file.\n\n");
                displayRunHelp();
            } else {
                cleaned_command = cleanCommand(command);
                user_job.est_run_time = atoi(cleaned_command);
                user_job.arg_list[1] = strdup(cleaned_command);
                command = strtok(NULL, " ");
                if(command == NULL) {
                    fprintf(stderr, "ERROR: You must specify a priority for the given file.\n\n");
                    displayRunHelp();
                } else {
                    cleaned_command = cleanCommand(command);
                    user_job.priority = atoi(cleaned_command);
                    user_job.unix_arrival_time = time(NULL);
                    localtime_r(&user_job.unix_arrival_time, &user_job.arrival_time);
                    user_job.is_running = 0;
                    
                    scheduler.job_cache = user_job;
                    total_num_of_jobs++;

                    printf("Job %s was submitted\n", user_job.job_name);
                    pthread_mutex_lock(&queue_mutex);
                    printf("Total number of jobs in the queue: %i\n", job_queue.queue_job_num+1);
                    pthread_mutex_unlock(&queue_mutex);

                    pthread_mutex_lock(&scheduler_mutex);
                    scheduler.expected_wait_time += user_job.est_run_time;
                    printf("Expected waiting time: %i seconds\n", scheduler.expected_wait_time);
                    printf("Scheduling Policy: %s\n\n", scheduler.policy);
                    pthread_mutex_unlock(&scheduler_mutex);

                    pthread_mutex_lock(&scheduler_condition_mutex);
                    pthread_cond_signal(&scheduler_queue_condition);
                    pthread_mutex_unlock(&scheduler_condition_mutex);

                }
            }
        }
    } else if(strcmp(cleaned_command, "list") == 0) {
        pthread_mutex_lock(&queue_mutex);
        printf("Total number of jobs in the queue: %i\n", job_queue.queue_job_num);
        int job_count;
        int job_index;        

        pthread_mutex_lock(&dispatcher_mutex); 
        if(dispatcher.queue_tail == JOB_QUEUE_MAX_SIZE-1) {
            job_index = 0;
        } else {
            job_index = dispatcher.queue_tail-1;
        }
        pthread_mutex_unlock(&dispatcher_mutex);

        printf("Name\t\tCPU_TIME\tPRI\tArrival_time\tProgress\n");
        for(job_count=0; job_count<job_queue.queue_job_num; job_count++) {
            printf("%.8s\t%i\t\t%i\t%02d:%02d:%02d\t", job_queue.queue[job_index].job_name, 
                                                       job_queue.queue[job_index].est_run_time,
                                                       job_queue.queue[job_index].priority,
                                                       job_queue.queue[job_index].arrival_time.tm_hour,
                                                       job_queue.queue[job_index].arrival_time.tm_min,
                                                       job_queue.queue[job_index].arrival_time.tm_sec);

            if(job_queue.queue[job_index].is_running == 1) {
                printf("%s\n", "Running");
            } else {
                printf("\n");
            }

            if(job_index >= JOB_QUEUE_MAX_SIZE-1) {
                job_index = 0;
            } else {
                job_index++;
            }
        }
        pthread_mutex_unlock(&queue_mutex);
        printf("Scheduling Policy: %s\n\n", scheduler.policy);   
       
    } else if(strcmp(cleaned_command, "fcfs") == 0) {
        pthread_mutex_lock(&scheduler_mutex);
        scheduler.policy = "FCFS";
        pthread_mutex_unlock(&scheduler_mutex);
        reallocateJobQueue();
        printf("Scheduling policy has been switched to FCFS. ");
        pthread_mutex_lock(&queue_mutex);
        if(job_queue.queue_job_num > 2) {
            printf("All the %i waiting jobs have been rescheduled\n\n", job_queue.queue_job_num-1);
        } else {
            printf("\n");
        }
        pthread_mutex_unlock(&queue_mutex);
    } else if(strcmp(cleaned_command, "sjf") == 0) {
        pthread_mutex_lock(&scheduler_mutex);
        scheduler.policy = "SJF";
        pthread_mutex_unlock(&scheduler_mutex);
        reallocateJobQueue();
        printf("Scheduling policy has been switched to SJF.\n");
        pthread_mutex_lock(&queue_mutex);
        if(job_queue.queue_job_num > 2) {
            printf("All the %i waiting jobs have been rescheduled\n\n", job_queue.queue_job_num-1);
        } else {
            printf("\n");
        }
        pthread_mutex_unlock(&queue_mutex);
    } else if(strcmp(cleaned_command, "priority") == 0) {
        pthread_mutex_lock(&scheduler_mutex);
        scheduler.policy = "priority";
        pthread_mutex_unlock(&scheduler_mutex);
        reallocateJobQueue();
        printf("Scheduling policy has been switched to Priority.\n");
        pthread_mutex_lock(&queue_mutex);
        if(job_queue.queue_job_num > 2) {
            printf("All the %i waiting jobs have been rescheduled\n\n", job_queue.queue_job_num-1);
        } else {
            printf("\n");
        }
        pthread_mutex_unlock(&queue_mutex);
    } else if(strcmp(cleaned_command, "test") == 0) {
        command = strtok(NULL, " ");
        if(command == NULL) {
            fprintf(stderr, "ERROR: You must specify a file to run.\n\n");
            displayTestHelp();
        } else {
            cleaned_command = cleanCommand(command);
            char* job_name = strdup(cleaned_command);
            command = strtok(NULL, " ");
            if(command == NULL) {
                fprintf(stderr, "ERROR: You must specify a policy to test.\n\n");
                displayTestHelp();
            } else {
                cleaned_command = cleanCommand(command);
                char* policy = strdup(cleaned_command);
                command = strtok(NULL, " ");
                if(command == NULL) {
                    fprintf(stderr, "ERROR: You must specify the number of jobs to insert.\n\n");
                    displayTestHelp();
                } else {
                    cleaned_command = cleanCommand(command);
                    int num_of_test_jobs = atoi(cleaned_command);
                    command = strtok(NULL, " ");
                    if(command == NULL) {
                        fprintf(stderr, "ERROR: You must specify an arrival rate for the jobs.\n\n");
                        displayTestHelp();
                    } else {
                        cleaned_command = cleanCommand(command);
                        float arrival_rate = atof(cleaned_command);
                        command = strtok(NULL, " ");
                        if(command == NULL) {
                            fprintf(stderr, "ERROR: You must specify the max number of priority levels.\n\n");
                            displayTestHelp();
                        } else {
                            cleaned_command = cleanCommand(command);
                            int max_priority_level = atoi(cleaned_command);
                            command = strtok(NULL, " ");
                            if(command == NULL) {
                                fprintf(stderr, "ERROR: You must specify the minimum cpu time allowed to be allocated.\n\n");
                                displayTestHelp();
                            } else {
                                cleaned_command = cleanCommand(command);
                                int min_cpu_time = atoi(cleaned_command);
                                command = strtok(NULL, " ");
                                if(command == NULL) {
                                    fprintf(stderr, "ERROR: You must specify the maximum cpu time allowed to be allocated.\n\n");
                                    displayTestHelp();
                                } else {
                                    cleaned_command = cleanCommand(command);
                                    int max_cpu_time = atoi(cleaned_command);
                                    
                                    tester.test_case.benchmark = job_name;
                                    tester.test_case.policy = policy;
                                    tester.test_case.num_of_jobs = num_of_test_jobs;
                                    tester.test_case.arrival_rate = arrival_rate;
                                    tester.test_case.max_priority_level = max_priority_level; 
                                    tester.test_case.min_cpu_time = min_cpu_time;
                                    tester.test_case.max_cpu_time = max_cpu_time;                   

                                    tester.test_started = 1;

                                    pthread_mutex_lock(&tester_condition_mutex);
                                    pthread_cond_signal(&tester_schedule_condition);
                                    pthread_mutex_unlock(&tester_condition_mutex);

                                    printf("---- Test Metrics ----\n");
                                    printf("Benchmark name: %s\n", job_name);
                                    printf("Policy: %s\n", policy);
                                    printf("Number of Jobs: %i\n", num_of_test_jobs);
                                    printf("Arrival rate: %f\n", arrival_rate);
                                    printf("Priority Levels: %i\n", max_priority_level);
                                    printf("Min Cpu Time: %i\n", min_cpu_time);
                                    printf("Max Cpu Time: %i\n\n", max_cpu_time);
                                }
                            }
                            
                        }
                    }
                }

            }
        }
    } else {
        printf("\tError: That command is not recognized. Type 'help' for a list of commands.\n\n");
    }

}


/* decided to use something like selection sort here where the minimum value will be 
 * dependent on what policy the scheduler is using. */
void reallocateJobQueue() {

    Job new_queue[JOB_QUEUE_MAX_SIZE];

    int job_count = 0;
    int job_index, min_index;

    pthread_mutex_lock(&queue_mutex);
    
    if(job_queue.queue_job_num > 1) {

        //printf("Reallocating job queue\n");

        // make a deep copy of the job_queue
        memcpy(new_queue, job_queue.queue, sizeof(job_queue.queue));

        pthread_mutex_lock(&dispatcher_mutex);
        job_index = dispatcher.queue_tail;

        while(job_count < job_queue.queue_job_num-1) {

            min_index = job_index;
            
            int job_sub_index;
            if(job_index == JOB_QUEUE_MAX_SIZE-1) {
                if(job_queue.queue[0].is_running) {
                    job_sub_index = 1;
                } else {
                    job_sub_index = 0;
                }
            } else { 
                job_sub_index = job_index+1;
            }
            
            int job_sub_count = job_count+1;
            while(job_sub_count <= job_queue.queue_job_num-2) {

                // here is where we check based on on different policies               
                pthread_mutex_lock(&scheduler_mutex);
                if(strcmp(scheduler.policy, "priority") == 0) {
                    if(new_queue[job_sub_index].priority > new_queue[min_index].priority) {
                        min_index = job_sub_index;
                    }
                } else if(strcmp(scheduler.policy, "FCFS") == 0) {
                    if(new_queue[job_sub_index].unix_arrival_time < new_queue[min_index].unix_arrival_time) {
                        min_index = job_sub_index;
                    }
                } else if(strcmp(scheduler.policy, "SJF") == 0) {
                    if(new_queue[job_sub_index].est_run_time < new_queue[min_index].est_run_time) {
                        min_index = job_sub_index;
                    }
                }
                pthread_mutex_unlock(&scheduler_mutex);

                if(job_sub_index == JOB_QUEUE_MAX_SIZE-1) {
                    job_sub_index = 0;
                } else {
                    job_sub_index++;
                }

                job_sub_count++;

            }
             
            // swap min and current index
            Job temp_job = new_queue[min_index];

            new_queue[min_index] = new_queue[job_index];
            new_queue[job_index] = temp_job; 

            if(job_index == JOB_QUEUE_MAX_SIZE-1) {
                job_index = 0;
            } else {
                job_index++;
            }


            job_count++;
        }         
        pthread_mutex_unlock(&dispatcher_mutex);
        memcpy(job_queue.queue, new_queue, sizeof(new_queue));
    }

    pthread_mutex_unlock(&queue_mutex);
    

}

char* cleanCommand(char* cmd) {

    int i;
    
    int letter_count = 0;

    for(i=0; i<strlen(cmd); i++) {
        if(cmd[i] != ' ' && cmd[i] != '\n') {
            cmd[letter_count] = cmd[i];
            letter_count++;
        }       
    }
    
    cmd[letter_count] = '\0';

    return cmd;
}

