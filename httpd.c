#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h> // for close
#include <stdlib.h>
#include <string.h> // for memset
#include <ctype.h>

void error_die(const char *e) {
    perror(e);
    exit(1);
}

int get_line(int fd, char *buff, int size, int index) {
    int i = index;
    char c = '\0';
    ssize_t n;
    while (i < size - 1 && c != '\n') {
        n = recv(fd, &c, 1, 0);
        if (n <= 0) {
            c = '\n';
            break;
        }
        buff[i] = c;
        i++;
    }
    buff[i] = '\0';
    return i;
}

// 判断是否是 header 结尾
int is_end(char *buff, int size) {
    if (size > 4 &&
        buff[size] == '\0' &&
        buff[size - 1] == '\n' &&
        buff[size - 2] == '\r' &&
        buff[size - 3] == '\n' &&
        buff[size - 4] == '\r') {
        return 1;
    }
    return 0;
}

int parse_url(char *buff, int idx, char *url, char *path, char *query) {

    int i = 0;
    char matchPath = '1';
    int path_len = 0;
    int query_len = 0;
    do {
        char c = buff[idx];
        url[i] = c;

        if (c == '?') {
            matchPath = '0';
            continue;
        }

        if (matchPath == '1') {
            path[path_len++] = c;
        } else {
            query[query_len++] = c;
        }
    } while (!isspace(buff[idx]) && ++idx && ++i);
    url[i] = '\0';
    path[path_len] = '\0';
    query[query_len - 1] = '\0';
    return idx;
}

int parse_method(char *buff, int idx, char *method) {
    int i = 0;
    do {
        method[i] = buff[idx];
    } while (!isspace(buff[idx]) && ++idx && ++i);
    method[i] = '\0';
    return idx;
}

void accept_request(void *args) {

    int fd = (int)args;

    char buff[1024];
    char method[255];
    char url[255];
    char path[255];
    char query[255];

    int line = 0;
    int index = 0;

    while (is_end(buff, index) == 0) {
        index = get_line(fd, buff, sizeof(buff), index);
        line++;
    }

    int idx = 0;
    idx = parse_method(buff, idx, method);
    idx = parse_url(buff, ++idx, url, path, query);

    printf("%s", buff);

    close(fd);
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

    // close(sock);

    return 0;
}
