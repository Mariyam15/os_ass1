#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>

#define PRODUCERS 2000
#define QUEUE_SIZE 8192
#define MSG_SIZE 600

typedef struct {
    int producer_id;
    char data[MSG_SIZE];
} log_item;

// Circular queue
log_item queue[QUEUE_SIZE];

// Atomic indices
atomic_int head = 0;
atomic_int tail = 0;

// Run flag
atomic_int running = 1;

FILE *logfile;

/* ---------------- PRODUCER ---------------- */
void* producer(void *arg) {
    int id = *(int*)arg;
    free(arg);

    while (atomic_load(&running)) {
        int t = atomic_load(&tail);
        int h = atomic_load(&head);

        // Check if queue is full
        if ((t + 1) % QUEUE_SIZE == h) {
            continue; // backpressure
        }

        log_item item;
        item.producer_id = id;
        snprintf(item.data, MSG_SIZE,
                 "Producer %d writing log\n", id);

        queue[t] = item;
        atomic_store(&tail, (t + 1) % QUEUE_SIZE);

        usleep(1000); // simulate workload
    }
    return NULL;
}

/* ---------------- CONSUMER ---------------- */
void* consumer(void *arg) {
    (void)arg;

    while (atomic_load(&running) ||
           atomic_load(&head) != atomic_load(&tail)) {

        int h = atomic_load(&head);
        int t = atomic_load(&tail);

        if (h == t) {
            continue; // queue empty
        }

        log_item item = queue[h];
        fprintf(logfile, "PID %d: %s",
                item.producer_id, item.data);

        atomic_store(&head, (h + 1) % QUEUE_SIZE);
    }
    return NULL;
}

/* ---------------- MAIN ---------------- */
int main() {
    pthread_t prod[PRODUCERS];
    pthread_t cons;

    logfile = fopen("logs.txt", "w");
    if (!logfile) {
        perror("fopen");
        return 1;
    }

    // Start consumer
    pthread_create(&cons, NULL, consumer, NULL);

    // Start producers
    for (int i = 0; i < PRODUCERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&prod[i], NULL, producer, id);
    }

    // Run for 30 minutes
    sleep(1800);
    atomic_store(&running, 0);

    // Join producers
    for (int i = 0; i < PRODUCERS; i++) {
        pthread_join(prod[i], NULL);
    }

    pthread_join(cons, NULL);
    fclose(logfile);

    return 0;
}
