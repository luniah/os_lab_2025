#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void demonstrate_zombie() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("ДОЧЕРНИЙ ПРОЦЕСС:\n");
        printf("PID: %d\n", getpid());
        printf("PPID: %d\n", getppid());
        printf("Завершаю работу через 3 секунды...\n");
        
        sleep(3);
        
        printf("Дочерний процесс завершен с кодом 42\n");
        exit(42); // Завершаем дочерний процесс
    }
    else if (pid > 0) {
        // Родительский процесс
        printf("РОДИТЕЛЬСКИЙ ПРОЦЕСС:\n");
        printf("PID: %d\n", getpid());
        printf("Дочерний PID: %d\n", pid);
        printf("НЕ вызываю wait() - создаю зомби!\n");
        printf("Родитель работает 10 секунд...\n");
        
        // НЕ вызываем wait() - специально создаем зомби
        for (int i = 0; i < 10; i++) {
            printf("Родитель работает... %d/10\n", i + 1);
            
            // Проверяем, стал ли дочерний процесс зомби
            char command[100];
            snprintf(command, sizeof(command), 
                    "ps -o pid,ppid,state,command -p %d 2>/dev/null", pid);
            
            if (i >= 3) { // После 3-й секунды проверяем зомби
                printf("\nПроверка состояния дочернего процесса:\n");
                system(command);
            }
            
            sleep(1);
        }
        
        printf("\nРодительский процесс завершается\n");
        printf("Зомби-процесс будет убран системой\n");
    }
    else {
        perror("Ошибка fork");
        exit(1);
    }
}

void demonstrate_proper_cleanup() {
    printf("\nПРАВИЛЬНАЯ ОБРАБОТКА ДОЧЕРНИХ ПРОЦЕССОВ:\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс %d: работаю...\n", getpid());
        sleep(2);
        printf("Дочерний процесс %d: завершаюсь\n", getpid());
        exit(123);
    }
    else if (pid > 0) {
        // Родительский процесс
        int status;
        printf("Родитель %d ожидает завершения дочернего...\n", getpid());
        
        pid_t finished_pid = wait(&status);
        
        if (finished_pid > 0) {
            if (WIFEXITED(status)) {
                printf("Дочерний процесс %d завершен с кодом: %d\n", 
                      finished_pid, WEXITSTATUS(status));
            }
        }
    }
}

void demonstrate_non_blocking_wait() {
    printf("\nНЕБЛОКИРУЮЩИЙ WAIT С WNOHANG:\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс: длительная работа (5 сек)...\n");
        sleep(5);
        exit(99);
    }
    else if (pid > 0) {
        // Родительский процесс
        int status;
        
        for (int i = 0; i < 8; i++) {
            pid_t result = waitpid(pid, &status, WNOHANG);
            
            if (result == 0) {
                printf("Дочерний процесс еще работает... (проверка %d)\n", i + 1);
            }
            else if (result == pid) {
                printf("Дочерний процесс завершен! Код: %d\n", WEXITSTATUS(status));
                break;
            }
            
            sleep(1);
        }
    }
}

int main() {
    printf("ДЕМОНСТРАЦИЯ ЗОМБИ-ПРОЦЕССОВ\n");
    
    demonstrate_zombie();
    demonstrate_proper_cleanup();
    demonstrate_non_blocking_wait();
    
    return 0;
}