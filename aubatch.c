#include <stdio.h>
#include "Scheduler.h"

int main() {

    printf("Hello from aubatch\n");

    Scheduler scheduler;

    scheduler.testName = "Josh";

    printf("%s\n", scheduler.testName);

    return 0;
}
