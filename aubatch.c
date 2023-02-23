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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int threadCounter = 0;

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
    for(count=0; count<5; count++) {
        pthread_mutex_lock(&mutex);
        threadCounter--;
        pthread_mutex_unlock(&mutex);
        printf("Scheduler: %i\n", threadCounter);
        sleep(1.2);
    }
}

void* dispatcherCount(void* ptr) {
    int count;
    for(count=0; count<5; count++) {
        pthread_mutex_lock(&mutex);
        threadCounter++;
        pthread_mutex_unlock(&mutex);
        printf("Dispatcher: %i\n", threadCounter);
        sleep(1);
    }
}
