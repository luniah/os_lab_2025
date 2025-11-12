#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

void print_usage(const char *progname) {
    printf("Usage: %s -a <IP> -p <port> -b <bufsize>\n", progname);
    printf("Options:\n");
    printf("  -a <IP>      Server IP address\n");
    printf("  -p <port>    Server port\n");
    printf("  -b <bufsize> Buffer size\n");
}

int main(int argc, char **argv) {
    int sockfd, n;
    char *sendline = NULL, *recvline = NULL;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    
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
    
    // Выделяем буферы
    sendline = (char *)malloc(bufsize);
    recvline = (char *)malloc(bufsize + 1);
    if (sendline == NULL || recvline == NULL) {
        perror("malloc");
        free(sendline);
        free(recvline);
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) < 0) {
        perror("inet_pton problem");
        free(sendline);
        free(recvline);
        exit(1);
    }
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket problem");
        free(sendline);
        free(recvline);
        exit(1);
    }

    write(1, "Enter string\n", 13);

    while ((n = read(0, sendline, bufsize)) > 0) {
        if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
            perror("sendto problem");
            break;
        }

        if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
            perror("recvfrom problem");
            break;
        }

        recvline[n] = '\0'; // Ensure null termination
        printf("REPLY FROM SERVER: %s\n", recvline);
    }
    
    free(sendline);
    free(recvline);
    close(sockfd);
    return 0;
}