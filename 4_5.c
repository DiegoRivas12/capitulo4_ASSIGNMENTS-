#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

// Task structure for the linked list
typedef struct Task {
    int value;                 // Task identifier (example: data for insertion)
    struct Task* next;         // Pointer to the next task
} Task;

// Global variables
Task* task_queue = NULL;       // Task queue (linked list)
pthread_mutex_t queue_mutex;   // Mutex to protect the task queue
pthread_cond_t task_cond;      // Condition variable for task availability
bool done = false;             // Flag indicating no more tasks will be generated

// Function prototypes
void* worker_thread(void* rank);
void add_task(int value);
Task* get_task(void);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int thread_count = strtol(argv[1], NULL, 10);
    pthread_t* threads = malloc(thread_count * sizeof(pthread_t));

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&task_cond, NULL);

    // Create worker threads
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_create(&threads[thread], NULL, worker_thread, (void*)thread);
    }

    // Generate tasks
    for (int i = 1; i <= 20; i++) { // Example: 20 tasks
        pthread_mutex_lock(&queue_mutex);
        add_task(i); // Add task to the queue
        printf("Main thread: Added task %d\n", i);
        pthread_cond_signal(&task_cond); // Signal a waiting thread
        pthread_mutex_unlock(&queue_mutex);
    }

    // Indicate no more tasks
    pthread_mutex_lock(&queue_mutex);
    done = true;
    pthread_cond_broadcast(&task_cond); // Wake up all threads
    pthread_mutex_unlock(&queue_mutex);

    // Wait for worker threads to finish
    for (int thread = 0; thread < thread_count; thread++) {
        pthread_join(threads[thread], NULL);
    }

    // Clean up
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&task_cond);
    free(threads);

    printf("Main thread: All tasks completed. Exiting.\n");
    return EXIT_SUCCESS;
}

// Worker thread function
void* worker_thread(void* rank) {
    long my_rank = (long)rank;

    while (true) {
        pthread_mutex_lock(&queue_mutex);

        // Wait for a task or the "done" signal
        while (task_queue == NULL && !done) {
            pthread_cond_wait(&task_cond, &queue_mutex);
        }

        if (done && task_queue == NULL) {
            pthread_mutex_unlock(&queue_mutex);
            break; // Exit the loop if no more tasks and "done" is true
        }

        Task* task = get_task(); // Get a task from the queue
        pthread_mutex_unlock(&queue_mutex);

        // Execute the task
        if (task != NULL) {
            printf("Thread %ld: Processing task %d\n", my_rank, task->value);
            free(task); // Free the task after processing
        }
    }

    printf("Thread %ld: Exiting.\n", my_rank);
    return NULL;
}

// Add a task to the queue
void add_task(int value) {
    Task* new_task = malloc(sizeof(Task));
    new_task->value = value;
    new_task->next = NULL;

    if (task_queue == NULL) {
        task_queue = new_task;
    } else {
        Task* temp = task_queue;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_task;
    }
}

// Get a task from the queue
Task* get_task(void) {
    if (task_queue == NULL) return NULL;

    Task* task = task_queue;
    task_queue = task_queue->next;
    return task;
}
