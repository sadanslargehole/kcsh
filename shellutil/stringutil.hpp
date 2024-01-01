#ifndef KCSH_STRINGUTIL
#define KCSH_STRINGUTIL

#include <sstream>
#include <string>
#include <vector>

inline std::vector<std::string> split(std::string inputString, char delimiter = ' ') {
    std::istringstream iss(inputString);
    std::vector<std::string> tokens;

    std::string token;
    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

inline std::vector<std::string> split(std::string* inputString, char delimiter = ' ') {
    return split(*inputString, delimiter);
}

inline std::string trim(const std::string& str) {
    size_t firstNonSpace = str.find_first_not_of(" \t\n\r");

    if (firstNonSpace == std::string::npos) {
        return "";
    }
    size_t lastNonSpace = str.find_last_not_of(" \t\n\r");
    return str.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
}

#endif