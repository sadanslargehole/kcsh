#ifndef KCSH_SHELLEXEC
#define KCSH_SHELLEXEC

#include "colors.hpp"
#include "settingsutil.hpp"
#include "stringutil.hpp"
#include "sysutil.hpp"
#include "vecutil.hpp"
#include <algorithm>
#include <asm-generic/errno-base.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include "../main.hpp"
namespace fs = std::filesystem;

inline int runCommand(char** args);

const std::vector<std::string> builtins = {
    "cd", "exit", "which",
    "kcshthemes", "settheme"}; // Remember to add your builtins!!!
inline void setSessionVar(std::string name, std::string val){
    shellVars[name] = val;
}
inline int shellexec(std::string cmd, char **args) {

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

inline int runCommand(std::string command, std::string* envName, std::string* envVal){
    if(envName == nullptr || envName->empty()){
        return runCommand((char**)cstringArray(split(command)).data());
    }
    std::string oldEnv = "";
    if(setenv(envName->c_str(), envVal->c_str(), 0)){
        oldEnv = getenv(envName->c_str());
    }
    setenv(envName->c_str(), envVal->c_str(), 1);
    auto a = split(command);
    auto b = cstringArray(a);
    auto x = const_cast<char**>(b.data());
auto toRet = runCommand(x);
    if(oldEnv.empty()){
        unsetenv(envName->c_str());
    }else{
        setenv(envName->c_str(), oldEnv.c_str(), 1);
    }
    return toRet;
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
    } else if (cmd == "kcshthemes") {
        std::string selected_theme = getenv("KCSH_THEME");
        if (selected_theme.empty()) selected_theme = "default";

        for (const auto& file : fs::directory_iterator(themesDir)) {
            std::string themename = replaceSubstring(file.path().filename().string(), ".ini", "");
            std::cout << themename << " " << (selected_theme == themename ? FG_GREEN + BOLD + "✓" + RESET : "") << std::endl;
        }
        return 0;
    } else if (cmd == "settheme") {
        std::string theme = (args + 1)[0];
        IniData settings = parseIniFile(settingsDir.string() + "/kcsh_config.ini");

        if (!fs::exists(themesDir.string() + "/" + theme + ".ini") ||
            !fs::is_regular_file(themesDir.string() + "/" + theme + ".ini")) {
                std::cout << "theme " << theme << " does not exist in " << themesDir.string() << std::endl;
                return ENOENT;
            }

        settings["appearance"]["theme"] = std::make_pair(theme, settings["appearance"]["theme"].second);
        saveIniFile(settings, settingsDir.string() + "/kcsh_config.ini");
        std::cout << "changed theme, restart for changes to take effect" << std::endl;
        return 0;
    } else {
        return WEXITSTATUS(shellexec(cmd, args));
    }
}
#endif
