#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define THREAD_COUNT 4  // Número de hilos

double a = 0.0, b = 3.0; // Límites de integración
int n = 1024;            // Número total de trapecios
double total_sum = 0.0;  // Suma compartida para la integral
pthread_mutex_t mutex;   // Mutex para exclusión mutua
sem_t semaphore;         // Semáforo para exclusión mutua
int lock = 0.0;
// Función que define f(x) para la integral
double f(double x) {
    return x * x; // Ejemplo: f(x) = x^2
}

// Regla del trapecio aplicada a una subregión
double Trap(double left_endpt, double right_endpt, int trap_count, double base_len) {
    double estimate = (f(left_endpt) + f(right_endpt)) / 2.0;
    for (int i = 1; i <= trap_count - 1; i++) {
        double x = left_endpt + i * base_len;
        estimate += f(x);
    }
    estimate *= base_len;
    return estimate;
}

// Estructura para los argumentos de cada hilo
typedef struct {
    int thread_id;  // Identificador del hilo
    int local_n;    // Número de trapecios asignados al hilo
    double h;       // Ancho de la base de los trapecios
} ThreadArgs;

// Implementación con exclusión mutua basada en busy-waiting
void* thread_busy_waiting(void* rank) {
    long my_rank = (long)rank;
    int n = 1024;
    double a = 0.0, b = 3.0;
    double h = (b - a) / n;
    int local_n = n / (long)pthread_self();
    double local_a = a + my_rank * local_n * h;
    double local_b = local_a + local_n * h;
    double local_sum = Trap(local_a, local_b, local_n, h);

    // Busy-waiting
    while (__sync_lock_test_and_set(&lock, 1)) {
        // Esperar hasta que el lock esté libre
        //Aquí, el hilo intenta adquirir el candado (lock). Si lock = 1, el hilo entra en un bucle esperando a que otro hilo libere el recurso.
    }

    total_sum += local_sum; // Sección crítica
    __sync_lock_release(&lock); // Liberar lock

    return NULL;
}

// Implementación con exclusión mutua basada en mutexes
void* thread_with_mutex(void* args) {
    ThreadArgs* my_args = (ThreadArgs*) args;
    int my_id = my_args->thread_id;
    int local_n = my_args->local_n;
    double h = my_args->h;
    double local_a = a + my_id * local_n * h;
    double local_b = local_a + local_n * h;

    double local_sum = Trap(local_a, local_b, local_n, h);

    // Bloquear el mutex antes de actualizar total_sum
    pthread_mutex_lock(&mutex);
    total_sum += local_sum;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

// Implementación con exclusión mutua basada en semáforos
void* thread_with_semaphore(void* args) {
    ThreadArgs* my_args = (ThreadArgs*) args;
    int my_id = my_args->thread_id;
    int local_n = my_args->local_n;
    double h = my_args->h;
    double local_a = a + my_id * local_n * h;
    double local_b = local_a + local_n * h;

    double local_sum = Trap(local_a, local_b, local_n, h);

    // Bloquear el semáforo antes de actualizar total_sum
    sem_wait(&semaphore);
    total_sum += local_sum;
    sem_post(&semaphore);

    return NULL;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    ThreadArgs thread_args[THREAD_COUNT];
    double h = (b - a) / n; // Ancho de cada trapecio
    int local_n = n / THREAD_COUNT;

    // Inicializar mutex y semáforo
    pthread_mutex_init(&mutex, NULL);
    sem_init(&semaphore, 0, 1);

    // Ejecutar con una de las tres implementaciones
    for (int thread = 0; thread < THREAD_COUNT; thread++) {
        thread_args[thread].thread_id = thread;
        thread_args[thread].local_n = local_n;
        thread_args[thread].h = h;
        
        // Alternar implementaciones:
        // pthread_create(&threads[thread], NULL, thread_busy_waiting, &thread_args[thread]);
        // pthread_create(&threads[thread], NULL, thread_with_mutex, &thread_args[thread]);
        pthread_create(&threads[thread], NULL, thread_with_semaphore, &thread_args[thread]);
    }

    // Esperar a que todos los hilos terminen
    for (int thread = 0; thread < THREAD_COUNT; thread++) {
        pthread_join(threads[thread], NULL);
    }

    printf("Con n = %d trapecios, la integral de %f a %f = %.15e\n", n, a, b, total_sum);

    // Liberar recursos
    pthread_mutex_destroy(&mutex);
    sem_destroy(&semaphore);

    return 0;
}
