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
    printf("\ttest <benchmark> <policy> <num_of_jobs> <priority_levels>\n\n");
    printf("\t\t<min_cpu_time> <max_cpu_time>\n\n");
}

void displayQuitHelp() {
    printf("\tquit: exit AUbatch\n\n");
}
