#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include "shellutil/stringutil.hpp"
#include "shellutil/vecutil.hpp"
#include "shellutil/colors.hpp"
#include "shellutil/sysutil.hpp"
#include "shellutil/shellexec.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

int main() {
    while (true) {

        std::string prompt_color        = FG_WHITE;
        std::string prompt_character    = "$";

        std::string cwd         = trim(fs::current_path());
        std::string cwd_color   = FG_BLUE;

        std::string user        = trim(sysexec("whoami"));
        std::string user_color  = FG_RED;

        std::string hostname        = trim(sysexec("cat /proc/sys/kernel/hostname"));
        std::string hostname_color  = FG_MAGENTA;

        std::string cmd = "";

        char** args;

        std::string formattedchar = prompt_color + prompt_character + " ";
        std::string formattedcwd  = prettyPath(cwd_color + cwd);
        std::string formatteduser = user_color + user;
        std::string formattedhost = hostname_color + hostname;

        std::string prompt ="[" + formatteduser + RESET + "@" + formattedhost + RESET + 
                            "] " + formattedcwd + RESET + "\n" + formattedchar + RESET;

        std::cout << prompt;
        
        std::getline(std::cin, cmd);
        if (std::cin.eof()){
            exit(0);
        }
        std::vector<std::string> tokens = split(&cmd);
        if (tokens.empty()) {
            continue;
        }

        std::vector<const char*> cstringTokens = cstringArray(tokens);
        cmd = cstringTokens[0];
        args = const_cast<char**>(cstringTokens.data());

        for (char** arg = args; *arg != nullptr; ++arg) {
            *arg = const_cast<char*>(replaceSubstring(*arg, "~", homePath().string()).c_str());
        }

        std::vector<std::string> builtins = {"cd", "exit", "which"}; // Remember to add your builtins!!!

        // UGHHHHH
        if (cmd == "cd") {

            fs::path path = join(args+1);
            
            path = path.is_absolute() ? path : fs::absolute(path);

            if (fs::exists(path) && fs::is_directory(path)) {
                fs::current_path(path);
            } else {
                errno = 2;
                perror("cd");
            }
        } else if (cmd == "exit") {
            exit(0);
        } else if (cmd == "which") {
            std::string execName = (args+1)[0];
            std::string execPath = findExecutablePath(execName);

            auto it = std::find(builtins.begin(), builtins.end(), execName);

            if (it != builtins.end()) {
                std::cout << execName << ": shell built-in command" << std::endl;
            } else if (!execPath.empty()) {
                std::cout << trim(execPath) << std::endl;
            } else {
                std::cout << execName << " not found" << std::endl;
            }
        } else {
            shellexec(cmd, args);
        }
    }
    
    return 0;
}