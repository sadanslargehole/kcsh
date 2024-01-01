#include <iostream>
#include <string>
#include "shellutil/stringutil.hpp"
#include "shellutil/vecutil.hpp"
#include "shellutil/colors.hpp"
#include "shellutil/sysexec.hpp"
#include <unistd.h>
#include <sys/wait.h>

int main() {
    std::string prompt_color        = FG_WHITE;
    std::string prompt_character    = "$";

    std::string cwd         = trim(sysexec("pwd"));
    std::string cwd_color   = FG_BLUE;

    std::string user        = trim(sysexec("whoami"));
    std::string user_color  = FG_RED;

    std::string hostname        = trim(sysexec("hostname"));
    std::string hostname_color  = FG_MAGENTA;

    while (true) {
        std::string cmd = "";

        std::string formattedchar = prompt_color + prompt_character + " ";
        std::string formattedcwd  = cwd_color + cwd;
        std::string formatteduser = user_color + user;
        std::string formattedhost = hostname_color + hostname;

        std::string prompt ="[" + formatteduser + RESET + "@" + formattedhost + RESET + 
                            "] " + formattedcwd + RESET + "\n" + formattedchar + RESET;

        std::cout << prompt;
        std::getline(std::cin, cmd);

        std::vector<std::string> tokens = split(&cmd);

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            return 1;
        } else if (pid == 0) {  // Child process
            std::vector<const char*> cstringTokens = cstringArray(tokens);
            execvp(cstringTokens[0], const_cast<char**>(cstringTokens.data()));
            
            perror("execvp");
            _exit(1);
        } else {  // Parent process
            // Wait for the child to finish
            int status;
            waitpid(pid, &status, 0);

            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                prompt_color = FG_RED;
            } else {
                prompt_color = FG_GREEN;
            }
        }
    }
    
    return 0;
}