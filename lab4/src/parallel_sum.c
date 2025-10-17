#include "utils.h"
#include "sum_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <time.h>


void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

void parse_arguments(int argc, char **argv, uint32_t *threads_num, uint32_t *array_size, uint32_t *seed) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--threads_num") == 0 && i + 1 < argc) {
      *threads_num = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--array_size") == 0 && i + 1 < argc) {
      *array_size = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
      *seed = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--help") == 0) {
      printf("Usage: %s --threads_num <num> --array_size <size> --seed <seed>\n", argv[0]);
      exit(0);
    }
  }
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;
  
  // Парсинг аргументов командной строки
  parse_arguments(argc, argv, &threads_num, &array_size, &seed);

  if (threads_num == 0 || array_size == 0) {
    printf("Error: threads_num and array_size must be positive\n");
    printf("Usage: %s --threads_num <num> --array_size <size> --seed <seed>\n", argv[0]);
    return 1;
  }
  
  printf("Configuration:\n");
  printf("  Threads: %u\n", threads_num);
  printf("  Array size: %u\n", array_size);
  printf("  Seed: %u\n", seed);

  pthread_t threads[threads_num];

  // Генерация массива
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  // Распределение работы между потоками
  struct SumArgs args[threads_num];
  int chunk_size = array_size / threads_num;
  int remainder = array_size % threads_num;
  int current_start = 0;
  
  for (uint32_t i = 0; i < threads_num; i++) {
    int chunk = chunk_size + (i < remainder ? 1 : 0);
    args[i].array = array;
    args[i].begin = current_start;
    args[i].end = current_start + chunk;
    current_start += chunk;
    
    printf("  Thread %u: elements [%d, %d)\n", i, args[i].begin, args[i].end);
  }

  // Замер времени начала вычислений
  clock_t start = clock();

  // Создание потоков
  for (uint32_t i = 0; i < threads_num; i++) {
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]) != 0) {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }

  // Сбор результатов
  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    void *result;
    if (pthread_join(threads[i], &result) != 0) {
      printf("Error: pthread_join failed!\n");
      free(array);
      return 1;
    }
    total_sum += (int)(size_t)result;
  }

  // Замер времени окончания вычислений
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

  free(array);
  printf("Total: %d\n", total_sum);
  printf("Execution time: %.6f seconds\n", elapsed);
  return 0;
}