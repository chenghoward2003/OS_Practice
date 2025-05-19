#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <bits/sigaction.h>  // Added for sigaction structure

#define BUFFER_SIZE 400  // Reduced from 20000 to match actual usage
#define CONSUMER 20
#define PRODUCER 50
#define CONSUME_AMOUNT 10
#define PRODUCE_AMOUNT 4

// Global variables
sem_t full, empty, mutex;
int buffer[BUFFER_SIZE];
int result[CONSUMER][CONSUME_AMOUNT];
int in = 0, out = 0;
volatile sig_atomic_t running = 1;

// Helper function for timing calculations
void calculate_times(clock_t start_time, double waiting_time, int thread_id, const char* thread_type) {
    clock_t end_time = clock();
    double turnaround_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    double burst_time = turnaround_time - waiting_time;

    printf("%s%2d - Waiting Time: %.6f sec, Burst Time: %.6f sec, Turnaround Time: %.6f sec\n",
           thread_type, thread_id, waiting_time, burst_time, turnaround_time);
    fflush(stdout);
}

// Signal handler for graceful shutdown
void signal_handler() {
    running = 0;
}

void* producer_func(void *arg) {
    clock_t start_time = clock();
    double waiting_time = 0;
    int producerID = *(int*)arg;
    int start = producerID * PRODUCE_AMOUNT;
    int end = start + PRODUCE_AMOUNT;

    for(int i = start; i < end && running; i++) {
        clock_t wait_start = clock();
        if (sem_wait(&empty) != 0) {
            perror("Error waiting on empty semaphore");
            pthread_exit(NULL);
        }
        if (sem_wait(&mutex) != 0) {
            perror("Error waiting on mutex");
            sem_post(&empty);
            pthread_exit(NULL);
        }
        clock_t wait_end = clock();
        waiting_time += ((double)(wait_end - wait_start)) / CLOCKS_PER_SEC;

        buffer[in] = i;
        printf("Producer%2d wrote %2d\n", producerID, i);
        fflush(stdout);
        in = (in + 1) % BUFFER_SIZE;

        if (sem_post(&mutex) != 0) {
            perror("Error posting mutex");
            pthread_exit(NULL);
        }
        if (sem_post(&full) != 0) {
            perror("Error posting full semaphore");
            pthread_exit(NULL);
        }
    }

    calculate_times(start_time, waiting_time, producerID, "Producer");
    pthread_exit(NULL);
}

void* consumer_func(void *arg) {
    clock_t start_time = clock();
    double waiting_time = 0;
    int consumerID = *(int*)arg;
    int item[CONSUME_AMOUNT];

    char fullPath[256];
    snprintf(fullPath, sizeof(fullPath), "%s/%d.txt", "out", consumerID);
    FILE* file = fopen(fullPath, "w");
    if (file == NULL) {
        perror("Error opening output file");
        pthread_exit(NULL);
    }

    for (int i = 0; i < CONSUME_AMOUNT && running; i++) {
        clock_t wait_start = clock();
        if (sem_wait(&full) != 0) {
            perror("Error waiting on full semaphore");
            fclose(file);
            pthread_exit(NULL);
        }
        clock_t wait_end = clock();
        waiting_time += ((double)(wait_end - wait_start)) / CLOCKS_PER_SEC;

        if (sem_wait(&mutex) != 0) {
            perror("Error waiting on mutex");
            sem_post(&full);
            fclose(file);
            pthread_exit(NULL);
        }

        item[i] = buffer[out];
        printf("Consumer%2d got %4d\n", consumerID, item[i]);
        result[consumerID][i] = item[i];
        fprintf(file, "Consumer%2d got %4d\n", consumerID, item[i]);
        fflush(stdout);
        out = (out + 1) % BUFFER_SIZE;

        if (sem_post(&mutex) != 0) {
            perror("Error posting mutex");
            fclose(file);
            pthread_exit(NULL);
        }
        if (sem_post(&empty) != 0) {
            perror("Error posting empty semaphore");
            fclose(file);
            pthread_exit(NULL);
        }
    }

    fclose(file);
    calculate_times(start_time, waiting_time, consumerID, "Consumer");
    pthread_exit(NULL);
}

int main() {
    pthread_t producers[PRODUCER], consumers[CONSUMER];
    int* producer_ids[PRODUCER];
    int* consumer_ids[CONSUMER];

    // Set up signal handler
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        return 1;
    }

    // Initialize semaphores with error checking
    if (sem_init(&empty, 0, BUFFER_SIZE) != 0) {
        perror("Error initializing empty semaphore");
        return 1;
    }
    if (sem_init(&full, 0, 0) != 0) {
        perror("Error initializing full semaphore");
        sem_destroy(&empty);
        return 1;
    }
    if (sem_init(&mutex, 0, 1) != 0) {
        perror("Error initializing mutex");
        sem_destroy(&empty);
        sem_destroy(&full);
        return 1;
    }

    // Create output directory
    if (mkdir("out", 0755) != 0 && errno != EEXIST) {
        perror("Error creating output directory");
        sem_destroy(&empty);
        sem_destroy(&full);
        sem_destroy(&mutex);
        return 1;
    }

    // Create producer threads
    for (int i = 0; i < PRODUCER; i++) {
        producer_ids[i] = malloc(sizeof(int));
        if (producer_ids[i] == NULL) {
            perror("Error allocating memory for producer ID");
            goto cleanup;
        }
        *producer_ids[i] = i;
        if (pthread_create(&producers[i], NULL, producer_func, producer_ids[i]) != 0) {
            perror("Error creating producer thread");
            free(producer_ids[i]);
            goto cleanup;
        }
    }

    // Create consumer threads
    for (int i = 0; i < CONSUMER; i++) {
        consumer_ids[i] = malloc(sizeof(int));
        if (consumer_ids[i] == NULL) {
            perror("Error allocating memory for consumer ID");
            goto cleanup;
        }
        *consumer_ids[i] = i;
        if (pthread_create(&consumers[i], NULL, consumer_func, consumer_ids[i]) != 0) {
            perror("Error creating consumer thread");
            free(consumer_ids[i]);
            goto cleanup;
        }
    }

    // Join threads
    for (int i = 0; i < PRODUCER; i++) {
        if (pthread_join(producers[i], NULL) != 0) {
            perror("Error joining producer thread");
        }
        free(producer_ids[i]);
    }

    for (int i = 0; i < CONSUMER; i++) {
        if (pthread_join(consumers[i], NULL) != 0) {
            perror("Error joining consumer thread");
        }
        free(consumer_ids[i]);
    }

    // Write results to file
    FILE *file = fopen("result.txt", "w");
    if (file == NULL) {
        perror("Error opening result file");
        goto cleanup;
    }

    for (int i = 0; i < CONSUMER; i++) {
        printf("Consumer %d got: ", i);
        fprintf(file, "Consumer %d got: ", i);
        for (int j = 0; j < CONSUME_AMOUNT; j++) {
            printf(" %d", result[i][j]);
            fprintf(file, " %d", result[i][j]);
        }
        printf("\n");
        fprintf(file, "\n");
    }
    fclose(file);

cleanup:
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);
    
    return 0;
}