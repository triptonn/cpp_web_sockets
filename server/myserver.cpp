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

    int max_fd = 100;
    timeval timeout;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int flags = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_return = bind(
        server_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));

    int listen_return = listen(server_fd, 100);

    server_log.write("Server starting on port 8080");

    fd_set read_fds;
    while (true) {
        std::cout << "server > " << std::flush;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(server_fd, &read_fds);

        for (int client_fd : client_fds) {
            FD_SET(client_fd, &read_fds);
        }

        max_fd = server_fd;
        for (int client_fd : client_fds) {
            max_fd = std::max(max_fd, client_fd);
        }
        max_fd = std::max(max_fd, STDIN_FILENO);

        select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_client_fd = accept(
                server_fd, reinterpret_cast<struct sockaddr *>(&client_addr),
                &client_len);
            if (new_client_fd < 0) {
                server_log.write("Failed to accept new client connection");
                continue;
            } else {
                std::string client_ip = inet_ntoa(client_addr.sin_addr);
                server_log.write("New client connected from " + client_ip +
                                 " with fd: " + std::to_string(new_client_fd));
            }

            client_fds.insert(client_fds.begin(), new_client_fd);
            server_log.write("New client connected: " +
                             std::to_string(new_client_fd));
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
                server_log.write("Server terminated by user");
                break;
            }
        }

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
    std::string end_msg = "Shutting down server\n";
    std::cout << end_msg;
    server_log.write(end_msg);
    return EXIT_SUCCESS;
}
