#include <stdio.h>
#include "Scheduler.h"
#include "Dispatcher.h"

int main() {

    printf("Hello from aubatch\n");

    Scheduler scheduler;

    scheduler.testName = "Scheduler";

    printf("%s\n", scheduler.testName);

    Dispatcher dispatcher;

    dispatcher.testName = "Dispatcher";

    printf("%s\n", dispatcher.testName);

    return 0;
}
