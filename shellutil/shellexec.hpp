#ifndef KCSH_SHELLEXEC
#define KCSH_SHELLEXEC

#include "settingsutil.hpp"
#include "stringutil.hpp"
#include "sysutil.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

const std::vector<std::string> builtins = {
    "cd", "exit", "which",
    "parseexampleini"}; // Remember to add your builtins!!!

inline int shellexec(std::string cmd, char **args,
                     char **environment = environ) {

    const char *command = cmd.c_str();

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    } else if (pid == 0) { // Child process

        execvp(command, args);
        perror("kcsh");
        _exit(1);
    } else { // Parent process
        // Wait for the child to finish
        int status;
        waitpid(pid, &status, 0);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return status;
        } else {
            return 0;
        }
    }
}
inline int runCommand(char **args) {
    std::string cmd = *args;
    // UGHHHHH
    if (cmd == "cd") {
        std::string params = join(args + 1);
        if (params.size() == 0) {
            setenv("OLDPWD", fs::current_path().c_str(), 1);
            fs::current_path(homePath());
            setenv("PWD", fs::current_path().c_str(), 1);
            return 0;
        } else if (params.compare("-") == 0) {
            char *dest = std::getenv("OLDPWD");
            setenv("OLDPWD", fs::current_path().c_str(), 1);
            fs::current_path(dest);
            setenv("PWD", fs::current_path().c_str(), 1);
            return 0;
        } else if (params.at(0) == '~') {
            params = replaceSubstring(params, "~", homePath());
        }
        fs::path path = params;

        path = path.is_absolute() ? path : fs::absolute(path);

        if (fs::exists(path) && fs::is_directory(path)) {
            setenv("OLDPWD", fs::current_path().c_str(), 1);
            fs::current_path(path);
            setenv("PWD", fs::current_path().c_str(), 1);
            return 0;
        } else {
            errno = 2;
            perror("cd");
            return 1;
        }
    } else if (cmd == "exit") {
        exit(0);
        return 0; // not sure if this is needed
    } else if (cmd == "which") {
        std::string execName = (args + 1)[0];
        std::string execPath = findExecutablePath(execName);

        auto it = std::find(builtins.begin(), builtins.end(), execName);

        if (it != builtins.end()) {
            std::cout << execName << ": shell built-in command" << std::endl;
            return 0;
        } else if (!execPath.empty()) {
            std::cout << trim(execPath) << std::endl;
            return 0;
        } else {
            std::cout << execName << " not found" << std::endl;
            return 1;
        }
    } else if (cmd == "parseexampleini") {
        IniData iniData = parseIniFile("example.ini");
        for (const auto &section : iniData) {
            std::cout << "section " << section.first << "\n";
            for (const auto &entry : section.second) {
                    std::cout << "key " << entry.first << ", value " << getIniValue(iniData, section.first, entry.first) << "\n";
            }
        }
        return 0;
    } else {
        return WEXITSTATUS(shellexec(cmd, args));
    }
}
#endif
