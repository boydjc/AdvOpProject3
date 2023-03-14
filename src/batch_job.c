#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int argc, char *argv[] )
{

  int time_to_sleep = atoi(argv[1]);

  //printf("Sleeping for %i seconds.\n", time_to_sleep);

  sleep(time_to_sleep);

  return 0;
}
