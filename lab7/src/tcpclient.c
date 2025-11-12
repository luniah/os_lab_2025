#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

void print_usage(const char *progname) {
    printf("Usage: %s -a <IP> -p <port> -b <bufsize>\n", progname);
    printf("Options:\n");
    printf("  -a <IP>      Server IP address\n");
    printf("  -p <port>    Server port\n");
    printf("  -b <bufsize> Buffer size\n");
}

int main(int argc, char *argv[]) {
    int fd;
    int nread;
    char *buf = NULL;
    struct sockaddr_in servaddr;
    
    // Параметры по умолчанию
    char *ip = NULL;
    int port = -1;
    int bufsize = -1;
    
    int opt;
    while ((opt = getopt(argc, argv, "a:p:b:")) != -1) {
        switch (opt) {
            case 'a':
                ip = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'b':
                bufsize = atoi(optarg);
                break;
            default:
                print_usage(argv[0]);
                exit(1);
        }
    }
    
    if (ip == NULL || port <= 0 || bufsize <= 0) {
        print_usage(argv[0]);
        exit(1);
    }
    
    // Выделяем буфер
    buf = (char *)malloc(bufsize);
    if (buf == NULL) {
        perror("malloc");
        exit(1);
    }

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creating");
        free(buf);
        exit(1);
    }

    memset(&servaddr, 0, SIZE);
    servaddr.sin_family = AF_INET;

    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
        perror("bad address");
        free(buf);
        exit(1);
    }

    servaddr.sin_port = htons(port);

    if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
        perror("connect");
        free(buf);
        exit(1);
    }

    write(1, "Input message to send\n", 22);
    while ((nread = read(0, buf, bufsize)) > 0) {
        if (write(fd, buf, nread) < 0) {
            perror("write");
            free(buf);
            exit(1);
        }
    }

    free(buf);
    close(fd);
    exit(0);
}