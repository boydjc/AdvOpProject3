#include <stdio.h> 
int main(int argc, char *argv[] )
{
  int i = 1;
  printf("A process is running ...\n");
  argc--;
  /* Print all the arguments passed from the commandline */
  while (argc > 0) {
    printf ("Argument %d is: %s\n", i, argv[i]);
    i++;
    argc--;
  }
  return 0;
}
