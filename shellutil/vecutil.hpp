#ifndef KCSH_VECUTIL
#define KCSH_VECUTIL

#include <string>
#include <unistd.h>
#include <vector>

inline std::vector<const char *> cstringArray(const std::vector<std::string> &strings) {
    std::vector<const char *> result;
    for (const auto &str : strings) {
        result.push_back(str.c_str());
    }
    result.push_back(nullptr);
    return result;
}

#endif