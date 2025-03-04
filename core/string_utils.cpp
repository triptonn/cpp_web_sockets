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

std::string percent_encoding(std::string string_value) {
    std::string processed_string;
    for (char c : string_value) {
        if (c == ' ') {
            processed_string += "%20";
        } else if (c == '+') {
            processed_string += "%2B";
        } else if (c == '!') {
            processed_string += "%21";
        } else if (c == '@') {
            processed_string += "%40";
        } else if (c == '#') {
            processed_string += "%23";
        } else if (c == '$') {
            processed_string += "%24";
        } else if (c == '%') {
            processed_string += "%25";
        } else if (c == '^') {
            processed_string += "%5E";
        } else if (c == '&') {
            processed_string += "%26";
        } else if (c == '<') {
            processed_string += "%3C";
        } else if (c == '>') {
            processed_string += "%3E";
        } else if (c == '*') {
            processed_string += "%2A";
        } else if (c == '(') {
            processed_string += "%28";
        } else if (c == ')') {
            processed_string += "%29";
        } else if (c == ',') {
            processed_string += "%2C";
        } else {
            processed_string += c;
        }
    }
    return processed_string;
}
