#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        printf("Example: %s 123 1000000\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Дочерний процесс - запускаем sequential_min_max
        printf("Child process: Starting sequential_min_max...\n");
        
        // Подготавливаем аргументы для exec
        char *exec_args[] = {
            "sequential_min_max",
            argv[1],  // seed
            argv[2],  // array_size
            NULL
        };
        
        // Запускаем программу
        execv("./sequential_min_max", exec_args);
        
        // Если execv вернул управление, значит произошла ошибка
        perror("execv failed");
        exit(1);
        
    } else {
        // Родительский процесс - ждем завершения дочернего
        printf("Parent process: Waiting for child (PID: %d) to finish...\n", pid);
        
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Parent process: Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Parent process: Child terminated by signal %d\n", WTERMSIG(status));
        }
        
        printf("Parent process: Finished\n");
    }
    
    return 0;
}