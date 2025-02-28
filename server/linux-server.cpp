// Copyright [2025] <Nicolas Selig>

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Modern C++ error handling using exceptions
[[noreturn]] void error(const std::string &msg) {
    throw std::runtime_error(msg + ": " + std::string(strerror(errno)));
}

int server_fd = -1;
int client_fd = -1;

void signal_handler(int signum) {
    std::cout << "\nShutting down server...";
    if (client_fd >= 0)
        close(client_fd);
    if (server_fd >= 0)
        close(server_fd);
    exit(0);
}

int main(int argc, char *argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "ERROR, no port provided\n";
            std::cerr << "Usage: " << argv[0] << " <port_number>\n";
            return 1;
        }

        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        // Add better port number validation
        int portno;
        try {
            portno = std::stoi(argv[1]);
            if (portno <= 0 || portno > 65535) {
                std::cerr << "ERROR: Port number must be between 1 and 65535\n";
                return 1;
            }
        } catch (const std::invalid_argument &e) {
            std::cerr << "ERROR: Invalid port number - must be a number\n";
            return 1;
        } catch (const std::out_of_range &e) {
            std::cerr << "ERROR: Port number out of range\n";
            return 1;
        }

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            error("Error opening socket");
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
            0) {
            error("ERROR setting socket options");
        }

        struct sockaddr_in serv_addr{};  // Zero-initialize using {}
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);

        if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&serv_addr),
                 sizeof(serv_addr)) < 0) {
            error("ERROR on binding");
        }

        while (true) {
            std::cout << "Waiting for connections...\n";
            listen(server_fd, 5);

            struct sockaddr_in cli_addr{};  // Zero-initialize using {}
            socklen_t clilen = sizeof(cli_addr);

            client_fd =
                accept(server_fd,
                       reinterpret_cast<struct sockaddr *>(&cli_addr), &clilen);

            if (client_fd < 0) {
                error("Error on accept");
            }

            bool isconnected = true;
            while (isconnected) {
                std::array<char, 256> buffer{};  // Zero-initialized buffer
                int n = read(client_fd, buffer.data(), buffer.size() - 1);
                if (n < 0) {
                    error("ERROR reading from socket");
                }

                if (buffer.data() == "abort") {
                    std::cout << "abort came through!\n";
                    std::cout << buffer.data() << '\n';
                    std::string response = "acknowledged";
                    n = write(client_fd, response.c_str(), response.length());
                    if (n < 0) {
                        error("ERROR writing to socket");
                    }

                    isconnected = false;

                    close(client_fd);
                    client_fd = -1;
                    close(server_fd);
                    server_fd = -1;

                } else {
                    std::cout << buffer.data() << '\n';
                    std::string response = "received";
                    n = write(client_fd, response.c_str(), response.length());
                    if (n < 0) {
                        error("ERROR writing to socket");
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        if (client_fd >= 0)
            close(client_fd);
        if (server_fd >= 0)
            close(server_fd);
        return 1;
    }
}
