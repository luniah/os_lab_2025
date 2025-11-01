#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

unsigned long long result = 1;  // общий результат вычислений
pthread_mutex_t mutex;         
int k, pnum, mod;               // число для факториала, кол-во потоков, модуль

void* worker(void* arg) {
    int thread_id = *(int*)arg;
    
    // количество чисел на поток
    int numbers_per_thread = k / pnum;
    // лишние числа, которые нужно распределить
    int remainder = k % pnum;
    
    // начальный и конечный индекс для потока
    int start = thread_id * numbers_per_thread + 1;
    int end = (thread_id + 1) * numbers_per_thread;
    
    // распределяем остаток
    if (thread_id < remainder) {
        start += thread_id;
        end += thread_id + 1;
    } else {
        start += remainder;
        end += remainder;
    }
    
    printf("Thread %d: numbers [%d, %d]\n", thread_id, start, end);
    
    unsigned long long local_result = 1;
    
    for (int i = start; i <= end; i++) {
        local_result = (local_result * i) % mod;  
    }
    
    pthread_mutex_lock(&mutex);                   
    result = (result * local_result) % mod;        // обновляем результат
    pthread_mutex_unlock(&mutex);                 
    
    return NULL;
}

int main(int argc, char** argv) {
    // значения по умолчанию
    k = 10;          
    pnum = 4;         
    mod = 1000000007; // большой модуль чтобы избежать переполнения
    
    // парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[++i]); 
            if (k < 0) {
                printf("Error: k must be non-negative\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--pnum") == 0 && i + 1 < argc) {
            pnum = atoi(argv[++i]);
            if (pnum <= 0) {
                printf("Error: pnum must be positive\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--mod") == 0 && i + 1 < argc) {
            mod = atoi(argv[++i]);
            if (mod <= 0) {
                printf("Error: mod must be positive\n");
                return 1;
            }
        }
    }
    
    // если потоков больше чем чисел - уменьшаем кол-во потоков
    if (pnum > k) {
        pnum = k;
        printf("Note: Using %d threads (k is too small for more threads)\n", pnum);
    }
    
    printf("Computing %d! mod %d using %d threads\n", k, mod, pnum);
    
    // факториал 0 и 1 всегда равен 1
    if (k == 0 || k == 1) {
        printf("Result: 1\n");
        return 0;
    }
    
    pthread_mutex_init(&mutex, NULL);
    
    pthread_t threads[pnum];  
    int thread_ids[pnum];     
    
    // создание потоков
    for (int i = 0; i < pnum; i++) {
        thread_ids[i] = i; 
        if (pthread_create(&threads[i], NULL, worker, &thread_ids[i]) != 0) {
            printf("Error: Failed to create thread %d\n", i);
            return 1;
        }
    }
    
    // ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            printf("Error: Failed to join thread %d\n", i);
        }
    }
    
    pthread_mutex_destroy(&mutex);
    
    printf("Result: %llu\n", result);
    
    return 0;
}
