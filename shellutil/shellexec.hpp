#ifndef KCSH_SHELLEXEC
#define KCSH_SHELLEXEC

#include "settingsutil.hpp"
#include "stringutil.hpp"
#include "sysutil.hpp"
#include "vecutil.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;
extern std::unordered_map<std::string, std::string> shellVars;

const std::vector<std::string> builtins = {
    "cd", "exit", "which", "export",
    "parseexampleini"}; // Remember to add your builtins!!!

inline int shellexec(std::string cmd, char **args,
                     char **environment = environ) {

    const char *command = cmd.c_str();

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    } else if (pid == 0) { // Child process
        execvpe(command, args, environment);
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

inline std::vector<char *> prepEnv(char *namesToAdd[], char *valuesToAdd[]) {
    std::vector<char *> toRet;
    for (char** i = environ; *i!=nullptr; i++) {
        toRet.push_back(*i);
    }
    int i = 0;
    while (*(namesToAdd +i) !=nullptr) {
        auto s = strlen(namesToAdd[i])+strlen(valuesToAdd[i])+2;
        char* x = new char[s]();
        // memset(x, 0, s);
        strcat(x, namesToAdd[i]);
        strcat(x, "=");
        strcat(x, valuesToAdd[i]);
        toRet.push_back(x);
        i++;
    }
    toRet.push_back(nullptr);
    return toRet;
}
inline int runCommand(char **args, char *envName[], char *envValue[]);
inline int runCommand(char **args, char *envName, char *envValue) {
    char *v[2] = {envName, nullptr};
    char *b[2] = {envValue, nullptr};
    return runCommand(args, v, b);
}
inline int runCommand(char **args, char *envName[], char *envValue[]) {

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
    } else if (cmd == "export") {
        // TODO: impliment export -p (prints the export commands for all env
        // vars)
        if (*(args + 1) == nullptr) {
            return 0;
        }
        if (strcmp(*(args+1), "-p") == 0){
            for(char** vars = environ; *vars !=nullptr;vars++){
                std::cout << "export " << *vars << '\n';
            }
            return 0;
        }
        bool willSet = false;
        std::string var;
        std::string val = "";
        for (char *arg = *(args + 1); *arg != 0x0; arg++) {
            if (willSet) {
                val += *arg;
            } else if (*arg == '=') {
                willSet = true;
            } else {
                var += *arg;
            }
        }
        {
            if (willSet) {
                setenv(var.c_str(), val.c_str(), 1);
                return 0;
            } else {
                if (shellVars.contains(var)) {
                    setenv(var.c_str(), shellVars[var].c_str(), 1);
                }
            }
            return 0;
        }

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
                std::cout << "key " << entry.first << ", value "
                          << getIniValue(iniData, section.first, entry.first)
                          << "\n";
            }
        }
        return 0;
    } else {
        if (envName == nullptr || *envName == nullptr)
            return WEXITSTATUS(shellexec(cmd, args));
        auto envToPass = prepEnv(envName, envValue);
        
        int exitStatus = WEXITSTATUS(
            shellexec(cmd, args, envToPass.data()));
        int i = 0;
        while(envName[i] !=nullptr){
            //-2 to avoid nullptr at end
            delete [] envToPass.at(envToPass.size()-2-i);
            i++;
        }
        return exitStatus;
    }
}
#endif