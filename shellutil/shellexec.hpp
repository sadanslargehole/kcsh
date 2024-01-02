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

inline int shellexec(std::string cmd, char** args, char** environment = environ) {

        const char* command = cmd.c_str();
        
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            return 1;
        } else if (pid == 0) {  // Child process

            execvp(command, args);

            perror("kcsh");
            _exit(1);
        } else {  // Parent process
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

#endif