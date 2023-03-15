/*
 * COMP7500/7506
 * Project 3: AUbatch - A Batch Scheduling System
 * 
 * Joshua Boyd
 * Department of Computer Science and Software Engineering
 * Auburn University
 * Mar. 14, 2023. Version 1.1
 *    
 * AUbatch is a CPU scheduling emulator. This file defines the menu displays.. 
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
#include <menu.h>

void displayGreeting() {
    printf("Welcome to Joshua Boyd's batch job scheduler Version 1.0\n");
    printf("Type 'help' to find more about AUbatch commands\n");
}

void displayHelp() {
    displayRunHelp();
    displayListHelp();
    displayFcfsHelp();
    displaySjfHelp();
    displayPriorityHelp();
    displayTestHelp();
    displayQuitHelp();
}

void displayRunHelp() {
    printf("\trun <job> <time> <pri>: submit a job named <job>, \n");
    printf("\t\t\t\texecution time is <time>, \n");
    printf("\t\t\t\tpriority time is <pri>\n\n");
}

void displayListHelp() {
    printf("\tlist: display the job status.\n\n");
}

void displayFcfsHelp() {
    printf("\tfcfs: change the scheduling policy to FCFS.\n\n");
}

void displaySjfHelp() {
    printf("\tsjf: change the scheduling policy to SJF.\n\n");
}

void displayPriorityHelp() {
    printf("\tpriority: change the scheduling policy to priority.\n\n");
}

void displayTestHelp() {
    printf("\ttest <benchmark> <policy> <num_of_jobs> <arrival_rate> <priority_levels>\n\n");
    printf("\t\t<min_cpu_time> <max_cpu_time>\n\n");
}

void displayQuitHelp() {
    printf("\tquit: exit AUbatch\n\n");
}
