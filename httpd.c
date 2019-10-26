#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h> // for close
#include <stdlib.h>
#include <string.h> // for memset
#include <ctype.h>
#include <sys/stat.h>

#define SERVER_STRING "Server: TinyHttpd/0.1.0\r\n"

char *document = ".";

void error_die(const char *e) {
    perror(e);
    exit(1);
}

int get_line(int fd, char *buf, int size, int index) {
    int i = index;
    char c = '\0';
    ssize_t n;
    while (i < size - 1 && c != '\n') {
        n = recv(fd, &c, 1, 0);
        if (n <= 0) {
            c = '\n';
            break;
        }
        buf[i] = c;
        i++;
    }
    buf[i] = '\0';
    return i;
}

// 判断是否是 header 结尾
int is_end(char *buf, int size) {
    if (size > 4 &&
        buf[size] == '\0' &&
        buf[size - 1] == '\n' &&
        buf[size - 2] == '\r' &&
        buf[size - 3] == '\n' &&
        buf[size - 4] == '\r') {
        return 1;
    }
    return 0;
}

int parse_url(char *buf, int idx, char *url, char *path, char *query) {

    int i = 0;
    char matchPath = '1';
    int path_len = 0;
    int query_len = 0;
    do {
        char c = buf[idx];
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
    } while (!isspace(buf[idx]) && ++idx && ++i);
    url[i] = '\0';
    path[path_len - 1] = '\0';
    if (query_len > 0) {
        query[query_len - 1] = '\0';
    } else {
        query[0] = '\0';
    }
    return idx;
}

int parse_method(char *buf, int idx, char *method) {
    int i = 0;
    do {
        method[i] = buf[idx];
    } while (!isspace(buf[idx]) && ++idx && ++i);
    method[i] = '\0';
    return idx;
}

void not_found(int fd) {

    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    sprintf(buf, "%s%s", buf, SERVER_STRING);
    sprintf(buf, "%sContent-Type: text/html\r\n", buf);
    sprintf(buf, "%s\r\n", buf);
    sprintf(buf, "%s<HTML><TITLE>Not Found</TITLE>\r\n", buf);
    sprintf(buf, "%s<BODY><P>404\r\n", buf);
    sprintf(buf, "%sSorry, Page Not Found :(\r\n", buf);
    sprintf(buf, "%s</BODY></HTML>\r\n", buf);
    send(fd, buf, strlen(buf), 0);
}

void headers(int fd, char *file_type) {

    char buf[1024];

    strcpy(buf, "HTTP/1.0 200 ok\r\n");
    sprintf(buf, "%s%s", buf, SERVER_STRING);
    sprintf(buf, "%sContent-type: %s\r\n", buf, file_type);
    sprintf(buf, "%s\r\n", buf);
    send(fd, buf, strlen(buf), 0);
}

void cat_file(int fd, FILE *file) {
    char buf[1024];
    do {
        fgets(buf, sizeof(buf), file);
        send(fd, buf, strlen(buf), 0);
    } while (!feof(file));
}

char *file_type(char *path) {
    char c;
    size_t length = strlen(path);
    size_t src_len = length;
    do {
        c = path[--src_len];
    } while (length > 0 && c != '.');

    char type[1024];
    int idx = 0;
    do {
        type[idx++] = path[++src_len];
    } while (src_len < length);
    if(idx == 1) type[0] = '\0';

    char *result = "text/html";
    if (strcmp(type, "css") == 0) {
        result = "text/css; charset=utf-8";
    } else if (strcmp(type, "js") == 0) {
        result = "application/javascript";
    } else if (strcmp(type, "png") == 0) {
        result = "image/png";
    } else if (strcmp(type, "jpg") == 0) {
        result = "image/jpeg";
    } else if (strcmp(type, "svg") == 0) {
        result = "image/svg+xml";
    } else if (strcmp(type, "ttf") == 0) {
        result = "font/ttf";
    } else if (strcmp(type, "ico") == 0) {
        result = "image/x-icon";
    }
    return result;
}

void serve_file(int fd, char *path) {
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        not_found(fd);
    } else {
        headers(fd, file_type(path));
        cat_file(fd, file);
    }
    fclose(file);
}

void execute_get(int fd, char *url, char *p, char *query) {
    int cgi = 0;
    if (query[0] != '\0')
        cgi = 1;

    char path[2048];
    sprintf(path, "%s%s", document, p);

    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");

    struct stat st;

    if (stat(path, &st) == -1)
        return not_found(fd);

    if ((st.st_mode & S_IFMT) == S_IFDIR)   // 是目录就补上默认文件
        strcat(path, "/index.html");

    // 判断是否是可执行文件
//    if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) {
//        cgi = 1;
//    }

    if (cgi == 1) {
        // todo: execute cgi
    } else {
        serve_file(fd, path);
    }
}

void execute_post(int fd, char *url, char *path) {

}

void accept_request(void *args) {

    int fd = (int)args;

    char buf[1024];
    char method[255];
    char url[1024];
    char path[1024];
    char query[255];

    int line = 0;
    int index = 0;

    while (is_end(buf, index) == 0) {
        index = get_line(fd, buf, sizeof(buf), index);
        line++;
    }

    int idx = 0;
    idx = parse_method(buf, idx, method);
    idx = parse_url(buf, ++idx, url, path, query);

    if (strcasecmp(method, "GET") == 0) {
        execute_get(fd, url, path, query);
    } else if (strcasecmp(method, "POST") == 0) {
        // todo: execute post
    } else {
        // unimplemented method
    }

//    printf("%s", buf);

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
