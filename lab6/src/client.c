#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mult_modulo.h"

#include <pthread.h>

struct Server {
  char ip[255];
  int port;
};

struct ThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

void *ServerRequest(void *args) {
  struct ThreadArgs *thread_args = (struct ThreadArgs *)args;
  
  struct hostent *hostname = gethostbyname(thread_args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
    pthread_exit(NULL);
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(thread_args->server.port);
  
  if (hostname->h_addr_list[0] != NULL) {
    memcpy(&server.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
  } else {
    fprintf(stderr, "No address found for %s\n", thread_args->server.ip);
    pthread_exit(NULL);
  }

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    pthread_exit(NULL);
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", thread_args->server.ip, thread_args->server.port);
    close(sck);
    pthread_exit(NULL);
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &thread_args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed to %s:%d\n", thread_args->server.ip, thread_args->server.port);
    close(sck);
    pthread_exit(NULL);
  }

  char response[sizeof(uint64_t)];
  int bytes_received = recv(sck, response, sizeof(response), 0);
  if (bytes_received < 0) {
    fprintf(stderr, "Receive failed from %s:%d\n", thread_args->server.ip, thread_args->server.port);
    close(sck);
    pthread_exit(NULL);
  } else if (bytes_received != sizeof(response)) {
    fprintf(stderr, "Incomplete data received from %s:%d\n", thread_args->server.ip, thread_args->server.port);
    close(sck);
    pthread_exit(NULL);
  }

  memcpy(&thread_args->result, response, sizeof(uint64_t));
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers_file[255] = {'\0'}; // 255 - максимальная длина пути к файлу

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) {
          fprintf(stderr, "Invalid k value\n");
          return 1;
        }
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) {
          fprintf(stderr, "Invalid mod value\n");
          return 1;
        }
        break;
      case 2:
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        servers_file[sizeof(servers_file) - 1] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // Чтение файла с серверами
  FILE *sf = fopen(servers_file, "r");
  if (!sf) {
    perror("Failed to open servers file");
    return 1;
  }

  struct Server *servers = NULL;
  size_t servers_num = 0;
  char line[255];
  
  while (fgets(line, sizeof(line), sf)) {
    // Пропускаем пустые строки и строки с пробелами
    if (strlen(line) <= 1) continue;
    
    servers_num++;
    servers = realloc(servers, sizeof(struct Server) * servers_num);
    if (!servers) {
      perror("Memory allocation failed");
      fclose(sf);
      return 1;
    }
    
    char *port_str = strchr(line, ':');
    if (!port_str) {
      fprintf(stderr, "Invalid server format: %s\n", line);
      free(servers);
      fclose(sf);
      return 1;
    }
    *port_str = '\0';
    port_str++;
    
    // Удаляем символ новой строки из port_str
    char *newline = strchr(port_str, '\n');
    if (newline) *newline = '\0';
    
    strncpy(servers[servers_num - 1].ip, line, sizeof(servers[0].ip) - 1);
    servers[servers_num - 1].ip[sizeof(servers[0].ip) - 1] = '\0';
    servers[servers_num - 1].port = atoi(port_str);
    
    if (servers[servers_num - 1].port <= 0) {
      fprintf(stderr, "Invalid port number: %s\n", port_str);
      free(servers);
      fclose(sf);
      return 1;
    }
  }
  fclose(sf);

  if (servers_num == 0) {
    fprintf(stderr, "No valid servers found\n");
    free(servers);
    return 1;
  }

  printf("Found %zu servers\n", servers_num);

  // Создание потоков для параллельного взаимодействия с серверами
  pthread_t threads[servers_num];
  struct ThreadArgs args[servers_num];
  uint64_t part = k / servers_num;

  for (int i = 0; i < servers_num; i++) {
    args[i].server = servers[i];
    args[i].begin = i * part + 1;
    args[i].end = (i == servers_num - 1) ? k : (i + 1) * part;
    args[i].mod = mod;
    args[i].result = 1; // Инициализируем результатом по умолчанию

    if (pthread_create(&threads[i], NULL, ServerRequest, (void *)&args[i])) {
      fprintf(stderr, "Thread creation failed\n");
      free(servers);
      return 1;
    }
  }

  uint64_t total = 1;
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    if (args[i].result != 1) { // Проверяем, что результат был получен
      total = MultModulo(total, args[i].result, mod);
    } else {
      fprintf(stderr, "Warning: No result from server %s:%d\n", 
              servers[i].ip, servers[i].port);
    }
  }

  printf("Final result: %llu\n", total);
  free(servers);
  return 0;
}