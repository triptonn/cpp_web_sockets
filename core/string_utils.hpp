// Copyright [2025] <Nicolas Selig>
//
//

#ifndef STRING_UTILS_HPP_
#define STRING_UTILS_HPP_

#include <map>
#include <string>

std::string format_header_name(std::string header_name);

enum class Mode { SPACES, DEFAULT, FULL };

std::string percent_encoding(const std::string &string_value);
std::string percent_encoding(const std::string &string_value,
                             const std::string &mode);

#endif  // STRING_UTILS_HPP_
