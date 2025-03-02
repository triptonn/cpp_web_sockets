// Copyright [2025] <Nicolas Selig>
//
//

#include <arpa/inet.h>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_fd, reinterpret_cast<struct sockaddr *>(&addr),
                sizeof(addr)) < 0) {
        std::cerr << "Connect failed\n";
        close(client_fd);
        return 1;
    }

    std::cout << "Connected to server\n";

    fd_set read_fds;
    char buffer[1024];
    while (true) {
        std::cout << "> " << std::flush;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(client_fd, &read_fds);

        int max_fd = std::max(STDIN_FILENO, client_fd);

        if (select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr) < 0) {
            std::cerr << "Select error\n";
            break;
        }

        // DEBUG: Print out descriptor information
        /* std::cout << "Select returned. Checking descriptors:\n";
        std::cout << "STDIN ready: " << FD_ISSET(STDIN_FILENO, &read_fds)
                  << "\n";
        std::cout << "Socket ready: " << FD_ISSET(client_fd, &read_fds) << "\n";
      */

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            std::string input;
            std::getline(std::cin, input);

            if (input == "quit") {
                break;
            }

            if (send(client_fd, input.c_str(), input.length(), 0) < 0) {
                std::cerr << "Failed to send message\n";
                break;
            }
        }

        if (FD_ISSET(client_fd, &read_fds)) {
            int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                std::cout << "Server disconnected\n";
                break;
            }

            buffer[bytes_received] = '\0';
            std::cout << "Server: " << buffer << std::endl;
        }
    }

    close(client_fd);
    return 0;
}
