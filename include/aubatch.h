#pragma once

int quit_flag;
int total_num_of_jobs;
int total_completed_jobs;

float total_turnaround_time;
float average_turnaround_time;

float total_wait_time;
float average_wait_time;

float total_cpu_time;
float average_cpu_time;

float total_response_time;
float average_response_time;

void init();
void parseUserCommand(char* userCommand);
char* cleanCommand(char* cmd);
void reallocateJobQueue();

extern int quit_flag;
extern int total_num_of_jobs;
extern int total_completed_jobs;

extern float total_turnaround_time;
extern float average_turnaround_time;

extern float total_wait_time;
extern float average_wait_time;

extern float total_cpu_time;
extern float average_cpu_time;

extern float total_response_time;
extern float average_response_time;
