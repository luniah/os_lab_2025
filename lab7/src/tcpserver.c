#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

void print_usage(const char *progname) {
    printf("Usage: %s -p <port> -b <bufsize>\n", progname);
    printf("Options:\n");
    printf("  -p <port>    Port to listen on\n");
    printf("  -b <bufsize> Buffer size\n");
}

int main(int argc, char *argv[]) {
    int lfd, cfd;
    int nread;
    char *buf = NULL;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    
    // Параметры по умолчанию
    int port = -1;
    int bufsize = -1;
    
    int opt;
    while ((opt = getopt(argc, argv, "p:b:")) != -1) {
        switch (opt) {
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
    
    if (port <= 0 || bufsize <= 0) {
        print_usage(argv[0]);
        exit(1);
    }
    
    const size_t kSize = sizeof(struct sockaddr_in);
    
    // Выделяем буфер
    buf = (char *)malloc(bufsize);
    if (buf == NULL) {
        perror("malloc");
        exit(1);
    }

    if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        free(buf);
        exit(1);
    }

    memset(&servaddr, 0, kSize);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
        perror("bind");
        free(buf);
        exit(1);
    }

    if (listen(lfd, 5) < 0) {
        perror("listen");
        free(buf);
        exit(1);
    }

    printf("TCP Server listening on port %d\n", port);

    while (1) {
        unsigned int clilen = kSize;

        if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
            perror("accept");
            continue;
        }
        printf("Connection established\n");

        while ((nread = read(cfd, buf, bufsize)) > 0) {
            write(1, buf, nread);
        }

        if (nread == -1) {
            perror("read");
        }
        close(cfd);
    }
    
    free(buf);
}