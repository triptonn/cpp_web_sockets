// Copyright [2025] <Nicolas Selig>
//
//

#pragma once
#include <chrono>
#include <fstream>
#include <string>

class Logger {
  private:
    std::ofstream log_file;
    std::string file_path;
    bool is_open{false};

    std::string get_timestamp() const;

  public:
    explicit Logger(const std::string &filename);
    ~Logger();

    void write(const std::string &msg);
    bool is_active() const { return is_open; }
};
