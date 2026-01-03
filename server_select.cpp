#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <cstring>

#define PORT 12345
#define BUF_SIZE 4096

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_SET(server_fd, &master);

    int max_fd = server_fd;
    char buffer[BUF_SIZE];

    while (true) {
        read_fds = master;
        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == server_fd) {
                    int client = accept(server_fd, NULL, NULL);
                    FD_SET(client, &master);
                    if (client > max_fd) max_fd = client;
                } else {
                    int bytes = recv(fd, buffer, BUF_SIZE, 0);
                    if (bytes <= 0) {
                        close(fd);
                        FD_CLR(fd, &master);
                    } else {
                        send(fd, buffer, bytes, 0);
                    }
                }
            }
        }
    }
}
