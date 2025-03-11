// Copyright [2025] <Nicolas Selig>
//
//

#include <signal.h>

#include <iostream>
#include <string>

#include "../core/http.hpp"

std::atomic<bool> running{true};

void signal_handler(int signal) {
    if (signal == SIGINT) {
        running.store(false);
    }
}

int main() {
    signal(SIGINT, signal_handler);

    try {
        HttpServer server(8080);
        server.register_get("/", [](const HttpRequest &req) {
            return HttpResponse::ok("Welcome to the test server");
        });

        server.register_get("/test", [](const HttpRequest &req) {
            return HttpResponse::ok("Test endpoint");
        });

        server.register_post("/echo", [](const HttpRequest &req) {
            return HttpResponse::ok(req.body);
        });

        server.start();
        std::cout << "Server running on port 8080. Press Ctrl+C to exit.\n";

        std::string input;
        while (running.load()) {
            std::cout << "server > " << std::flush;
            std::getline(std::cin, input);

            if (input == "quit" || !running.load()) {
                break;
            }
            if (input == "status") {
                std::cout << "Server is running\n";
            }
        }

        server.stop();
        std::cout << "\nServer shutdown complete\n";
    } catch (const std::exception &e) {
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
