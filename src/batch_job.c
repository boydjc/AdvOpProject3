#include <stdio.h> 
int main(int argc, char *argv[] )
{
    if(argc >= 2) {
        int seconds_to_sleep = atoi(argv[1]);
        printf("%i\n", seconds_to_sleep);
        printf("Sleeping for %i seconds.\n", seconds_to_sleep);
        sleep(seconds_to_sleep);
    }

  return 0;
}
