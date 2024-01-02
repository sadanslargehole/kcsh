#ifndef KCSH_SYSUTIL
#define KCSH_SYSUTIL

#include <iostream>
#include <cstdio>
#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <filesystem>
#include "stringutil.hpp"

namespace fs = std::filesystem;

inline std::string sysexec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }
    return result;
}

// Attempt to get the current user's home directory. $HOME or current directory if nonexistent.
inline fs::path homePath() {
    fs::path home = std::getenv("HOME");
    return home;
}

// Compress user's home path into a tilde for display
inline std::string prettyPath(std::string path) {
    return replaceSubstring(path, homePath(), "~");
}

inline std::string prettyPath(fs::path path) {
    return prettyPath(path.string());
}

#endif