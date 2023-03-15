/*
 * COMP7500/7506
 * Project 3: AUbatch - A Batch Scheduling System
 *  
 * Joshua Boyd
 * Department of Computer Science and Software Engineering
 * Auburn University
 * Mar. 14, 2023. Version 1.1
 *    
 * AUbatch is a CPU scheduling emulator. This file defines an instance of the Job structure. 
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

#include <job.h>


Job user_job;
