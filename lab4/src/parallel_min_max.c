#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для хранения PID дочерних процессов
pid_t *child_pids = NULL;
int timeout = 0; // Таймаут в секундах (0 - отключен)
volatile sig_atomic_t timeout_occurred = 0;

// Обработчик сигнала SIGALRM
void timeout_handler(int sig) {
    timeout_occurred = 1;
    printf("Timeout reached! Sending SIGKILL to all child processes.\n");
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  // Добавляем опцию timeout
  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4:  // timeout
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            printf("Timeout set to %d seconds\n", timeout);
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"]\n",
           argv[0]);
    return 1;
  }

  // Выделяем память для хранения PID дочерних процессов
  child_pids = malloc(sizeof(pid_t) * pnum);
  if (child_pids == NULL) {
    printf("Failed to allocate memory for child PIDs\n");
    return 1;
  }

  // Устанавливаем обработчик сигнала SIGALRM если задан таймаут
  if (timeout > 0) {
    if (signal(SIGALRM, timeout_handler) == SIG_ERR) {
        perror("Failed to set signal handler");
        free(child_pids);
        return 1;
    }
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int pipes[2 * pnum];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes + i * 2) < 0) {
        printf("Failed to create pipe\n");
        free(child_pids);
        free(array);
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int chunk_size = array_size / pnum;

  // Запускаем таймер если задан таймаут
  if (timeout > 0) {
    alarm(timeout);
  }

  // Создаем дочерние процессы
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      child_pids[i] = child_pid; // Сохраняем PID дочернего процесса
      
      if (child_pid == 0) {
        // child process
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : start + chunk_size;
        
        struct MinMax local_min_max = GetMinMax(array, start, end);

        if (with_files) {
          // use files here
          char filename[32];
          snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
          FILE *file = fopen(filename, "w");
          if (file != NULL) {
            fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
            fclose(file);
          }
        } else {
          // use pipe here
          close(pipes[i * 2]); 
          write(pipes[i * 2 + 1], &local_min_max.min, sizeof(int));
          write(pipes[i * 2 + 1], &local_min_max.max, sizeof(int));
          close(pipes[i * 2 + 1]); 
        }
        free(array);
        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      free(child_pids);
      free(array);
      return 1;
    }
  }

  // Parent process
  free(array);

  // Ожидаем завершения дочерних процессов с возможностью таймаута
  while (active_child_processes > 0) {
    if (timeout_occurred) {
      // Таймаут произошел - убиваем все дочерние процессы
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] > 0) {
          printf("Killing child process %d\n", child_pids[i]);
          kill(child_pids[i], SIGKILL);
        }
      }
      break;
    }
    
    // Неблокирующее ожидание
    int status;
    pid_t finished_pid = waitpid(-1, &status, WNOHANG);
    
    if (finished_pid > 0) {
      // Дочерний процесс завершился
      active_child_processes -= 1;
      
      // Находим и отмечаем завершенный процесс
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] == finished_pid) {
          child_pids[i] = 0; // Помечаем как завершенный
          break;
        }
      }
      
      if (WIFEXITED(status)) {
        printf("Child process %d exited normally with status %d\n", 
               finished_pid, WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
        printf("Child process %d killed by signal %d\n", 
               finished_pid, WTERMSIG(status));
      }
    } else if (finished_pid == 0) {
      // Нет завершенных процессов - небольшая пауза
      usleep(100000); // 100ms
    } else {
      perror("waitpid failed");
      break;
    }
  }

  // Если был таймаут, ждем завершения всех убитых процессов
  if (timeout_occurred) {
    printf("Waiting for killed processes to terminate...\n");
    while (active_child_processes > 0) {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Собираем результаты только от процессов, которые успели завершиться
  for (int i = 0; i < pnum; i++) {
    // Пропускаем процессы, которые были убиты по таймауту
    if (timeout_occurred && child_pids[i] != 0) {
      printf("Skipping results from killed process %d\n", child_pids[i]);
      continue;
    }
    
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // read from files
      char filename[32];
      snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
      FILE *file = fopen(filename, "r");
      if (file != NULL) {
        fscanf(file, "%d %d", &min, &max);
        fclose(file);
        remove(filename); 
      }
    } else {
      // read from pipes
      close(pipes[i * 2 + 1]); 
      read(pipes[i * 2], &min, sizeof(int));
      read(pipes[i * 2], &max, sizeof(int));
      close(pipes[i * 2]); 
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  if (timeout_occurred) {
    printf("WARNING: Calculation terminated by timeout after %d seconds\n", timeout);
    printf("Results may be incomplete!\n");
  }
  
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  // Очистка
  free(child_pids);
  fflush(NULL);
  return 0;
}