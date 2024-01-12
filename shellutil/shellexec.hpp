#ifndef KCSH_SHELLEXEC
#define KCSH_SHELLEXEC

#include "colors.hpp"
#include "settingsutil.hpp"
#include "stringutil.hpp"
#include "sysutil.hpp"
#include <algorithm>
#include <asm-generic/errno-base.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ios>
#include <istream>
#include <ostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include <unordered_map>

namespace fs = std::filesystem;

const std::vector<std::string> builtins = {
    "cd", "exit", "which","export","unset",
    "kcshthemes", "settheme"}; // Remember to add your builtins!!!

extern std::unordered_map<std::string, std::string> shellVars;

inline int shellexec(std::string cmd, char **args, char** environment = environ) {

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
    }else if (cmd == "unset"){
        return 0;
    }
    else if (cmd == "export") {
        // TODO: impliment export -p (prints the export commands for all env
        // vars)
        if (*(args + 1) == nullptr) {
            return 0;
        }
        if (strcmp(*(args+1), "-p") == 0){
            for(char** vars = environ; *vars !=nullptr;vars++){
                
                for(char* sp = *vars; *sp!=0x0;sp++){
                    if(*sp == ' '){
                        
                    }
                }
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
        if (*(args+1) == nullptr){
            return 1;
        }
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
inline int runCommand(char** args){
    return runCommand(args, (char**) nullptr, (char**)nullptr);
}
//returns nullptr on error
inline int* runPipe(char **one, char **two, char *envName[], char* envValue[]){
    int* retVals= new int[2];
    pid_t pid_1 = fork();
    if(pid_1 <0){
        perror("fork");
        return nullptr;
    }else if (pid_1 == 0) {
        int out = dup(1);
        pid_t pid_2 = fork();
        if(pid_2 <0){
            perror("fork 2");
            exit(1);
        }else if (pid_2 == 0) {
            dup2(out, 0);
            retVals[1] = runCommand(two);
            _exit(1);
        }else{
            retVals[0] = runCommand(one, envName, envValue);
            waitpid(pid_2, nullptr, 0);
            _exit(1);
        }
    }else {
        waitpid(pid_1, nullptr, 0);
        return retVals;
    }
}
inline int outToAppend(char** command,const char* fileName, char *envName, char *envValue){
    FILE* out;
    out = fopen(fileName, "a");
    pid_t a = fork();
    if (a<0){
        perror("fork");
        return 1;
    }else if (a == 0){
        dup2(fileno(out), 1);
        runCommand(command, envName, envValue);
        close(fileno(out));
        _exit(1);
    }else{
        waitpid(a, nullptr, 0);
    }
    fclose(out);
    return 0;
}
inline int outToFile(char **command, char *fileName, char *envName[], char *envValue[]){
    return 0;
}
#endif