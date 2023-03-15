/*
 * COMP7500/7506
 * Project 3: AUbatch - A Batch Scheduling System
 *  
 * Joshua Boyd
 * Department of Computer Science and Software Engineering
 * Auburn University
 * Mar. 14, 2023. Version 1.1
 *   
 * AUbatch is a CPU scheduling emulator. This file defines the batch job to be
 * ran with AUBatch. 
 *  
 * Compilation Instruction:
 *     Compile using the 'make batch' command*  
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int argc, char *argv[] )
{

  int time_to_sleep = atoi(argv[1]);

  sleep(time_to_sleep);

  return 0;
}
