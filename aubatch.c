#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct Scheduler {
    char* policyName;
} Scheduler;

typedef struct Dispatcher {

} Dispatcher;

void* schedulerModule();
void* dispatcherModule();

pthread_cond_t counterCondition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t conditionMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counterMutex = PTHREAD_MUTEX_INITIALIZER;

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

    while(threadCounter <= 10) {

        pthread_mutex_lock(&counterMutex);
        threadCounter++;
        pthread_mutex_unlock(&counterMutex);

        printf("Scheduler: %i\n", threadCounter);
        
        pthread_mutex_lock(&conditionMutex);
        pthread_cond_signal(&counterCondition);
        pthread_mutex_unlock(&conditionMutex);

        if(threadCounter < 10) {
            pthread_mutex_lock(&conditionMutex);
            pthread_cond_wait(&counterCondition, &conditionMutex);
            pthread_mutex_unlock(&conditionMutex);
        }
    }
}

void* dispatcherCount(void* ptr) {

    while(threadCounter <= 10) {

        pthread_mutex_lock(&counterMutex);
        threadCounter++;
        pthread_mutex_unlock(&counterMutex);

        printf("Dispatcher: %i\n", threadCounter);

        pthread_mutex_lock(&conditionMutex);
        pthread_cond_signal(&counterCondition);
        pthread_mutex_unlock(&conditionMutex);

        if(threadCounter <= 10) {
            pthread_mutex_lock(&conditionMutex);
            pthread_cond_wait(&counterCondition, &conditionMutex);
            pthread_mutex_unlock(&conditionMutex);
        }
    }
    
}
