#include <job_queue.h>

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

JobQueue job_queue;