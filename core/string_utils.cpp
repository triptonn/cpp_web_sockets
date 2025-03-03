// Copyright [2025] <Nicolas Selig>
//
//

#include <string>

#include "string_utils.hpp"

std::string format_header_name(std::string header_name) {
    std::string formatted_header_name;
    bool capitalize = true;
    for (char c : header_name) {
        if (capitalize && std::isalpha(c)) {
            formatted_header_name += std::toupper(c);
            capitalize = false;
        } else if (c == '-') {
            formatted_header_name += c;
            capitalize = true;
        } else {
            formatted_header_name += c;
        }
    }
    return formatted_header_name;
}

std::string handle_space_for_path(std::string string_value) {
    std::string processed_string;
    for (char c : string_value) {
        if (c == ' ') {
            processed_string += "%20";
        } else {
            processed_string += c;
        }
    }
    return processed_string;
}
