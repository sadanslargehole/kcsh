#ifndef KCSH_SHELLEXEC
#define KCSH_SHELLEXEC

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <sys/wait.h>
#include <filesystem>
#include <unistd.h>
#include "stringutil.hpp"

namespace fs = std::filesystem;

inline int shellexec(std::string cmd, char** args, char** environment) {

        const char* command = cmd.c_str();
        
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            return 1;
        } else if (pid == 0) {  // Child process

            execvp(command, args);

            perror("execvp");
            _exit(1);
        } else {  // Parent process
            // Wait for the child to finish
            int status;
            waitpid(pid, &status, 0);

            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                std::string path = join(args+1);
                std::cout << path << std::endl;

                if (std::strcmp(command, "cd")) {
                    if (fs::exists(path) && fs::is_directory(path)) {
                        putenv(const_cast<char*>(("PWD=" + path).c_str()));
                    }
                }
                return status;
            } else {
                return 0;
            }
        }
}

#endif