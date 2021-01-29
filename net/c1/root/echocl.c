#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE 512

void set_socket_unblock(const int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int main(int argc, char** argv) {
    struct sockaddr_in addr; /* для адреса сервера */
    socklen_t addrlen; /* размер структуры с адресом */
    int       sk;      /* файловый дескриптор сокета */
    char      buf[BUFSIZE] = {0}; /* буфер для сообщений */
    char      recv_buf[BUFSIZE] = {0}; /* буфер для сообщений от сервера */
    int       len;

    if (argc != 3) {
        printf("Usage: echoc <ip>\nEx.:   echoc 10.30.0.2 client1\n");
        exit(0);
    }

    int name_len = strlen(argv[2]);
    if (name_len > 20) {
        printf("Client name is too long. Max 20 chars\n");
        exit(0);
    }
    // Сообщения будут начинаться с имени клиента.
    memcpy(buf, argv[2], name_len);
    buf[name_len++] = ':';
    buf[name_len++] = ' ';

    // Сделаем файловый дескриптор стандартного ввода неблокируемым.int flag =
    set_socket_unblock(STDIN_FILENO);

    /* создаём TCP-сокет */
    if ((sk = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port        = htons(1996);

    /* соединяемся с сервером */
    if (connect(sk, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }

    set_socket_unblock(sk);

    fd_set read_fds;
    printf("Connected to Echo server. Type /q to quit.\n");
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sk, &read_fds);

        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        // Есть данные на стандартном вводе.
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            fgets(buf + name_len, BUFSIZE - name_len - 1, stdin);

            len          = strlen(buf);
            buf[len - 1] = 0;
            if (strcmp(buf, "/q") == 0)
                break;
            if (send(sk, buf, len, 0) < 0) {
                perror("send");
                exit(1);
            }
        }

        // Есть данные от сервера.
        if (FD_ISSET(sk, &read_fds)) {
            len = recv(sk, recv_buf, BUFSIZE, 0);
            if (len < 0) {
                perror("recv");
                exit(1);
            } else if (len == 0) {
                printf("Remote host has closed the connection\n");
                exit(1);
            }

            printf("%s\n", recv_buf);
        }
    }
    return 0;
}
