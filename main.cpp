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

int main()
{

    setenv("OLDPWD", fs::current_path().c_str(), 1);
    setenv("PWD", fs::current_path().c_str(), 1);
    while (true)
    {

        std::string prompt_color = FG_WHITE;
        std::string prompt_character = "$";

        std::string cwd = trim(fs::current_path());
        std::string cwd_color = FG_BLUE;

        std::string user = trim(sysexec("whoami"));
        std::string user_color = FG_RED;

        std::string hostname = trim(sysexec("cat /proc/sys/kernel/hostname"));
        std::string hostname_color = FG_MAGENTA;

        std::string cmd = "";

        char **args;

        std::string formattedchar = prompt_color + prompt_character + " ";
        std::string formattedcwd = prettyPath(cwd_color + cwd);
        std::string formatteduser = user_color + user;
        std::string formattedhost = hostname_color + hostname;

        std::string prompt = "[" + formatteduser + RESET + "@" + formattedhost + RESET +
                             "] " + formattedcwd + RESET + "\n" + formattedchar + RESET;

        std::cout << prompt;

        std::getline(std::cin, cmd);
        if (std::cin.eof())
        {
            exit(0);
        }
        std::vector<std::string> tokens = split(&cmd);
        if (tokens.empty())
        {
            continue;
        }

        std::vector<const char *> cstringTokens = cstringArray(tokens);
        cmd = cstringTokens[0];
        args = const_cast<char **>(cstringTokens.data());
        // FIXME: are some of these vectors redundant
        // FIXME: add support for quotes and escaping
        std::vector<char*> command;
        for (char** arg = args; *arg != nullptr; ++arg)
        {
            if (strcmp(*arg, "||")==0)
            {
                if (runCommand(command.data()) != 0)
                {
                    std::cout << "||" << std::endl;
                    command.clear();
                    continue;
                }
                break;
            }
            else if (strcmp(*arg, ";")==0)
            {
                std::cout << ";" << std::endl;
                runCommand(command.data());
                command.clear();
                continue;
            }
            else if (strcmp(*arg, "&&")==0)
            {
                std::cout << "&&"<< std::endl;
                if (runCommand(command.data()) == 0)
                {
                    command.clear();
                    continue;
                }
                break;
            }
            else
            {
                command.push_back(*arg);
            }
        }
        // run command if no thingiers were found
        if (command.size()>0){
            runCommand(command.data());
        }
    }

    return 0;
}