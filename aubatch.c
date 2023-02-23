#include <stdio.h>

typedef struct Scheduler {
    char* policyName;
} Scheduler;

typedef struct Dispatcher {

} Dispatcher;

void schedulerCount();
void dispatcherCount();

int main() {

    printf("Hello from aubatch\n");

    Scheduler scheduler;

    scheduler.policyName = "FCFS";

    schedulerCount();
    dispatcherCount();

    Dispatcher dispatcher;

    return 0;
}

void schedulerCount() {
    int count;
    for(count=0; count<10; count++) {
        printf("Scheduler: %i\n", count);
    }
}

void dispatcherCount() {
    int count;
    for(count=0; count<10; count++) {
        printf("Dispatcher: %i\n", count);
    }
}
