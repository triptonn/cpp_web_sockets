// Copyright [2025] <Nicolas Selig>
//
//

#include "logger.hpp"
#include <iomanip>
#include <sstream>

Logger::Logger(const std::string &filename) : file_path(filename) {
    log_file.open(filename, std::ios::app);
    is_open = log_file.is_open();
}

Logger::~Logger() {
    if (is_open) {
        log_file.close();
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::write(const std::string &msg) {
    if (is_open) {
        log_file << "[" << get_timestamp() << "] " << msg << std::endl;
    }
}
