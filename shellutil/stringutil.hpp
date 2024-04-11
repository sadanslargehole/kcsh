#ifndef KCSH_STRINGUTIL
#define KCSH_STRINGUTIL

#include <iostream>
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

inline std::string join(char** strings, char delimiter = char()) {
    std::string result = "";
    if(*strings==nullptr){
        return result;
    }
    for (char** currentString = strings; *currentString != nullptr; ++currentString) {
        result += *currentString;
        
        if (*(currentString + 1) != nullptr) {
            result += delimiter;
        }
    }

    return result;
}

inline std::string join(const std::vector<std::string>& strings, char delimiter = char()) {
    std::string result = "";

    for (auto currentString = strings.begin(); currentString != strings.end(); ++currentString) {
        result += *currentString;

        if (std::next(currentString) != strings.end()) {
            result += delimiter;
        }
    }

    return result;
}

inline std::string replaceSubstring(const std::string& original, const std::string& toReplace, const std::string& replacement) {
    std::string result = original;
    size_t position = result.find(toReplace);

    while (position != std::string::npos) {
        result.replace(position, toReplace.length(), replacement);
        position = result.find(toReplace, position + replacement.length());
    }

    return result;
}

inline std::string replaceEnvironmentVariables(const std::string& input) {
    std::string output;
    size_t startPos = 0;
    size_t dollarPos = input.find('$');
    
    while (dollarPos != std::string::npos) {
        output += input.substr(startPos, dollarPos - startPos);
        
        size_t endPos = input.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_", dollarPos + 1);
        if (endPos == std::string::npos)
            endPos = input.size();
        
        std::string placeholder = input.substr(dollarPos + 1, endPos - dollarPos - 1);
        
        char* value = std::getenv(placeholder.c_str());
        if (value != nullptr) {
            output += value;
        } else {
            output += '$' + placeholder;
        }
        
        startPos = endPos;
        dollarPos = input.find('$', startPos);
    }
    
    output += input.substr(startPos);
    
    return output;
}


#endif