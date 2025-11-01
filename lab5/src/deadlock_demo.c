#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Два мьютекса для создания deadlock
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Функция для потока 1
void* thread1_function(void* arg) {
    printf("Thread 1: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Locked mutex1\n");
    
    // Искусственная задержка чтобы гарантировать deadlock
    sleep(1); // Даем время thread2 захватить mutex2
    
    printf("Thread 1: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);  // Блокировка - mutex2 уже захвачен thread2
    printf("Thread 1: Locked mutex2\n");
    
    // Критическая секция (никогда не выполнится из-за deadlock)
    printf("Thread 1: Critical section\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

// Функция для потока 2
void* thread2_function(void* arg) {
    printf("Thread 2: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Locked mutex2\n");
    
    sleep(1); // Даем время thread1 захватить mutex1
    
    printf("Thread 2: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);  // Блокировка - mutex1 уже захвачен thread1
    printf("Thread 2: Locked mutex1\n");
    
    // Критическая секция 
    printf("Thread 2: Critical section\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("DEADLOCK DEMONSTRATION\n\n");
    
    // Создаем потоки
    if (pthread_create(&thread1, NULL, thread1_function, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }
    
    if (pthread_create(&thread2, NULL, thread2_function, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }
    
    // Даем потокам время на выполнение
    sleep(3);
    
    // Проверяем, завершились ли потоки
    int try_join1 = pthread_tryjoin_np(thread1, NULL);
    int try_join2 = pthread_tryjoin_np(thread2, NULL);
    
    if (try_join1 != 0 || try_join2 != 0) {
        printf("\nDEADLOCK DETECTED!\n");
        printf("Threads are blocked waiting for each other's mutexes.\n");
        printf("The program is stuck in deadlock and will not terminate normally.\n");
        
        printf("Killing the program...\n");
        exit(1);
    }
    
    // Этот код никогда не выполнится из-за deadlock
    printf("Program finished normally (this should not happen)\n");
    
    return 0;
}