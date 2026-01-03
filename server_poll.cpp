#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>
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

    std::vector<pollfd> fds;
    fds.push_back({server_fd, POLLIN, 0});

    char buffer[BUF_SIZE];

    while (true) {
        poll(fds.data(), fds.size(), -1);

        for (size_t i = 0; i < fds.size(); i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd) {
                    int client = accept(server_fd, NULL, NULL);
                    fds.push_back({client, POLLIN, 0});
                } else {
                    int bytes = recv(fds[i].fd, buffer, BUF_SIZE, 0);
                    if (bytes <= 0) {
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        i--;
                    } else {
                        send(fds[i].fd, buffer, bytes, 0);
                    }
                }
            }
        }
    }
}
