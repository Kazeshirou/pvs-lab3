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

#define BUFSIZE    512
#define MAX_CLIENT 10

int clients[MAX_CLIENT] = {0};

void close_client_socket(const int client_fd) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] < 0) {
            continue;
        }

        if (clients[i] == client_fd) {
            clients[i] = -1;
            close(client_fd);
            break;
        }
    }
}

void send_to_all_other_clients(const char* msg, const int size,
                               const int current_client) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] < 0) {
            continue;
        }

        if (clients[i] == current_client) {
            continue;
        }

        printf("send \"%s\" to %d\n", msg, clients[i]);
        if (send(clients[i], msg, size, 0) < 0) {
            perror("send");
        }
    }
}

void set_socket_unblock(const int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

/* обслуживание одного клиента */
void serve_client(int sk) {
    printf("serve %d\n", sk);
    char buf[BUFSIZE];
    int  len;
    while (1) {
        len = recv(sk, buf, BUFSIZE, 0);

        if (len < 0) {
            if (errno == EWOULDBLOCK) {
                return;
            }

            close_client_socket(sk);
            perror("recv");
            return;
        }

        if (len == 0) {
            printf("Remote host has closed the connection\n");
            close_client_socket(sk);
            return;
        }

        buf[len] = '\0';
        printf("received %s\n", buf);

        send_to_all_other_clients(buf, len, sk);
    }
}

void clear_clients() {
    for (int i = 0; i < MAX_CLIENT; i++) {
        clients[i] = -1;
    }
}

void add_clients_to_fdset(fd_set* fdset) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] < 0) {
            continue;
        }

        FD_SET(clients[i], fdset);
    }
}

int add_client(const int new_client) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] >= 0) {
            continue;
        }

        clients[i] = new_client;
        set_socket_unblock(new_client);
        return 0;
    }
    return -1;
}


int create_server_socket() {
    /* создаём TCP-сокет */
    int sk = socket(AF_INET, SOCK_STREAM, 0);
    if (sk < 0) {
        perror("socket");
        exit(1);
    }

    // Для повторного использования локального адреса при перезапуске
    // сервера до истечения требуемого времени ожидания.
    int on = 1;
    if (setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(sk);
        exit(-1);
    }


    /* указываем адрес и порт */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port        = htons(1996);

    /* связываем сокет с адресом и портом */
    if (bind(sk, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    /* переводим сокет в состояние ожидания соединений  */
    if (listen(sk, SOMAXCONN) < 0) {
        perror("listen");
        exit(1);
    };

    set_socket_unblock(sk);
    return sk;
}

void serve_server(int sk) {
    struct sockaddr_in sender;
    socklen_t          addrlen = sizeof(sender);
    int                client = accept(sk, (struct sockaddr*)&sender, &addrlen);

    if (client < 0) {
        perror("accept");
        exit(1);
    }

    if (add_client(client) < 0) {
        close(client);
    }
}

int main(void) {
    int sk;     /* файловый дескриптор сокета */
    int client; /* сокет клиента */

    sk = create_server_socket();
    printf("server socket %d\n", sk);

    printf("Echo server listening --- press Ctrl-C to stop\n");
    clear_clients();
    fd_set read_fds;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sk, &read_fds);
        add_clients_to_fdset(&read_fds);

        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        if (FD_ISSET(sk, &read_fds)) {
            serve_server(sk);
        }

        for (int i = 0; i < MAX_CLIENT; i++) {
            if (clients[i] < 0) {
                continue;
            }
            if (!FD_ISSET(clients[i], &read_fds)) {
                continue;
            }

            serve_client(clients[i]);
        }
    }
    return 0;
}
