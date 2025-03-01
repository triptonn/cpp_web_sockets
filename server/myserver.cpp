// Copyright [2025] <Nicolas Selig>
//
//

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "../core/logger.hpp"

std::vector<int> client_fds;
const int MAX_FD = FD_SETSIZE;

int main() {
    Logger server_log("server.log");

    short int max_fd = 100;
    timeval timeout;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int flags = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_return = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));

    int listen_return = listen(server_fd, 100);

    server_log.write("Server starting on port 8080");

    fd_set read_fds;
    while (true) {
        std::cout << "server > " << std::flush;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(server_fd, &read_fds);

        if (FD_ISSET(server_fd, &read_fds)) {
            int new_client_fd = accept(server_fd, nullptr, nullptr);
            client_fds.insert(client_fds.begin(), new_client_fd);
            server_log.write("New client connected: " +
                             std::to_string(new_client_fd));
        }

        for (int client_fd : client_fds) {
            FD_SET(client_fd, &read_fds);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            std::string input;
            std::getline(std::cin, input);

            if (input == "quit") {
                for (int client_fd : client_fds) {
                    if (send(client_fd, "SERVEREXIT", 10, 0)) {
                        std::cerr << "Failed to send exit call\n";
                    }
                }
                break;
            }
        }

        select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

        for (std::vector<int>::iterator it = client_fds.begin();
             it != client_fds.end();) {
            int client_fd = *it;
            if (FD_ISSET(client_fd, &read_fds)) {
                char buffer[1024];
                int bytes_received =
                    recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received <= 0) {
                    server_log.write("Client disconnected: " +
                                     std::to_string(client_fd));
                    close(client_fd);
                    it = client_fds.erase(it);
                } else {
                    buffer[bytes_received] = '\0';
                    std::cout << "Received: " << buffer << std::endl;
                    std::string client = std::to_string(client_fd);

                    std::string log_entry = "Client " + client + ": " + buffer;
                    server_log.write(log_entry);
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
    std::cout << "Shutting down server\n";
    return EXIT_SUCCESS;
}
