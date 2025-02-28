// Copyright [2025] <Nicolas Selig>

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Modern C++ error handling using exceptions
[[noreturn]] void error(const std::string &msg) {
    throw std::runtime_error(msg + ": " + std::string(strerror(errno)));
}

int main(int argc, char *argv[]) {
    try {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " hostname port\n";
            return 1;
        }

        // Port number validation
        int portno;
        try {
            portno = std::stoi(argv[2]);
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

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            error("ERROR opening socket");
        }

        // Get host by name
        struct hostent *server = gethostbyname(argv[1]);
        if (server == nullptr) {
            throw std::runtime_error("ERROR: no such host");
        }

        struct sockaddr_in serv_addr{};  // Zero-initialize using {}
        serv_addr.sin_family = AF_INET;
        std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr,
                    server->h_length);
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd, reinterpret_cast<struct sockaddr *>(&serv_addr),
                    sizeof(serv_addr)) < 0) {
            error("ERROR connecting");
        }

        bool isconnected = true;
        while (isconnected) {
            std::cout << "Please enter the message: ";
            std::array<char, 256> buffer{};  // Zero-initialized buffer

            // Read user input (safely)
            if (!std::cin.getline(buffer.data(), buffer.size())) {
                error("ERROR reading from stdin");
            }

            if (buffer.data() == "abort") {
                isconnected = false;
            }

            // Write to socket
            ssize_t n =
                write(sockfd, buffer.data(), std::strlen(buffer.data()));
            if (n < 0) {
                error("ERROR writing to socket");
            }

            // Clear buffer for receiving response
            buffer.fill(0);
            n = read(sockfd, buffer.data(), buffer.size() - 1);
            if (n < 0) {
                error("ERROR reading from socket");
            }

            std::cout << buffer.data() << '\n';
        }
        close(sockfd);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
