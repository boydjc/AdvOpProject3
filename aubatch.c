#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct Scheduler {
    char* policyName;
} Scheduler;

typedef struct Dispatcher {

} Dispatcher;

void* schedulerCount();
void* dispatcherCount();

int main() {

    pthread_t schedulerThread, dispatcherThread;

    int iret1 = pthread_create(&schedulerThread, NULL, schedulerCount, NULL);
    int iret2 = pthread_create(&dispatcherThread, NULL, dispatcherCount, NULL); 

    pthread_join(schedulerThread, NULL);
    pthread_join(dispatcherThread, NULL);

    printf("Hello from aubatch\n");

    Scheduler scheduler;

    scheduler.policyName = "FCFS";

    Dispatcher dispatcher;

    return 0;
}

void* schedulerCount(void* ptr) {
    int count;
    for(count=0; count<10; count++) {
        printf("Scheduler: %i\n", count);
        sleep(1.2);
    }
}

void* dispatcherCount(void* ptr) {
    int count;
    for(count=0; count<10; count++) {
        printf("Dispatcher: %i\n", count);
        sleep(1);
    }
}
