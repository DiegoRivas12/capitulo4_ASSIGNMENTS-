#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Variables globales
float* data;        // Datos de entrada
int data_count;     // Número de datos
float min_meas;     // Valor mínimo de la medición
float max_meas;     // Valor máximo de la medición
int bin_count;      // Número de bins
float* bin_maxes;   // Límite superior de cada bin
int* global_bin_counts; // Histograma global
int thread_count;   // Número de hilos
pthread_mutex_t mutex; // Mutex para proteger el acceso al histograma global

// Función que encuentra el bin adecuado para un valor dado
int find_bin(float value) {
    for (int b = 0; b < bin_count; b++) {
        if (b == 0) {
            if (value >= min_meas && value < bin_maxes[b]) return b;
        } else {
            if (value >= bin_maxes[b - 1] && value < bin_maxes[b]) return b;
        }
    }
    return bin_count - 1; // En caso de ser el valor máximo
}

// Función que cada hilo ejecutará
void* thread_histogram(void* rank) {
    long my_rank = (long) rank;
    int local_n = data_count / thread_count; // Datos por hilo
    int my_first = my_rank * local_n;
    int my_last = (my_rank + 1) * local_n - 1;

    // Contadores locales del histograma
    int* local_bin_counts = calloc(bin_count, sizeof(int));

    // Clasificar los datos de mi porción
    for (int i = my_first; i <= my_last; i++) {
        int bin = find_bin(data[i]);
        local_bin_counts[bin]++;
    }

    // Agregar los resultados al histograma global
    pthread_mutex_lock(&mutex);
    for (int b = 0; b < bin_count; b++) {
        global_bin_counts[b] += local_bin_counts[b];
    }
    pthread_mutex_unlock(&mutex);

    free(local_bin_counts);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <número de hilos>\n", argv[0]);
        exit(1);
    }

    thread_count = strtol(argv[1], NULL, 10);//10   es  la base numerica que se debe usar para interpretar el número en la cadena. 

    // Datos de ejemplo
    float example_data[] = {1.3, 2.9, 0.4, 0.3, 1.3, 4.4, 1.7, 0.4, 3.2, 0.3,
                            4.9, 2.4, 3.1, 4.4, 3.9, 0.4, 4.2, 4.5, 4.9, 0.9};
    data_count = sizeof(example_data) / sizeof(example_data[0]);
    data = example_data;
    min_meas = 0.0;
    max_meas = 5.0;
    bin_count = 5;

    // Inicializar bins y contadores
    float bin_width = (max_meas - min_meas) / bin_count;
    bin_maxes = malloc(bin_count * sizeof(float));
    global_bin_counts = calloc(bin_count, sizeof(int));
    for (int b = 0; b < bin_count; b++) {
        bin_maxes[b] = min_meas + bin_width * (b + 1);
    }

    // Inicializar mutex
    pthread_mutex_init(&mutex, NULL);

    // Crear hilos
    pthread_t* thread_handles = malloc(thread_count * sizeof(pthread_t));
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, thread_histogram, (void*) thread);
    }

    // Esperar a que los hilos terminen
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL);
    }

    // Imprimir el histograma
    printf("Histograma:\n");
    for (int b = 0; b < bin_count; b++) {
        printf("Bin %d (%.1f - %.1f): %d\n", b,
               b == 0 ? min_meas : bin_maxes[b - 1],
               bin_maxes[b], global_bin_counts[b]);
    }

    // Liberar memoria y destruir el mutex
    free(bin_maxes);
    free(global_bin_counts);
    free(thread_handles);
    pthread_mutex_destroy(&mutex);

    return 0;
}
