#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h> // for close
#include <stdlib.h>
#include <string.h> // for memset

void error_die(const char *e) {
    perror(e);
    exit(1);
}

void accept_request(void *args) {
    printf("%lu\n", sizeof(args));
}

/// 起一个socket
int startup(int *port) {
    int httpd = socket(AF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket startup failed!");

    int on = 1;
    if ((setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
        error_die("socket setsockopt failed");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;  // IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   // 转为网络字节序 long
    addr.sin_port = htons(*port);   // 转为网络字节序 short

    if (bind(httpd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        error_die("socket bind failed!");

    if (listen(httpd, 5) < 0)
        error_die("socket listen failed!");

    return httpd;
}

int main() {
    int port = 2233;

    int sock = startup(&port);
    printf("httpd running on port %d\n", port);

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    pthread_t newThread;

    int client_sock = -1;

    while (1) {
        client_sock = accept(sock, (
        struct sockaddr *)&addr, &addr_len);

        if (client_sock == -1)
            error_die("socket accept failed!");

        if (pthread_create(&newThread, NULL, (void *)accept_request, (void *)(intptr_t)client_sock) != 0 )
            perror("accept request pthread create error!");
    }

    close(sock);

    return 0;
}