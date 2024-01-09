#include "shellutil/colors.hpp"
#include "shellutil/settingsutil.hpp"
#include "shellutil/shellexec.hpp"
#include "shellutil/stringutil.hpp"
#include "shellutil/sysutil.hpp"
#include "shellutil/vecutil.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

int main([[gnu::unused]] int argc, char** argv) {

    setenv("OLDPWD", fs::current_path().c_str(), 1);
    setenv("PWD", fs::current_path().c_str(), 1);
    fs::path settingsDir = homePath().string() + "/.config/kcsh/";
    fs::path themesDir   = settingsDir.string() + "/themes/";
    
    IniData settings;
    IniData theme;

    fs::create_directory(homePath().string() + "/.config");
    fs::create_directory(settingsDir);
    fs::create_directory(themesDir);

    settings = parseIniFile(settingsDir.string() + "/kcsh_config.ini");
    
    if (settings.empty()) {
        #ifdef NDEBUG
            std::cout << "Creating default config in " + settingsDir.string() << std::endl;
        #endif
        settings = getDefaultConfig();

        saveIniFile(settings, settingsDir.string() + "/kcsh_config.ini");
    }

    fs::path themePath = themesDir.string() + "/" + getIniValue(settings, "appearance", "theme") + ".ini";
    theme = parseIniFile(themePath);
    
    if (theme.empty()) {
        #ifdef NDEBUG
            std::cout << "Creating " + getIniValue(settings, "appearance", "theme") + " theme in " + themesDir.string() << std::endl;
        #endif
        theme = getDefaultTheme();
        saveIniFile(theme, themePath);
    }

    while (true) {

        std::string prompt_color        = FG_WHITE;
        std::string prompt_character    = getIniValue(theme, "prompt", "promptcharacter") + " ";

        std::string cwd         = prettyPath(trim(fs::current_path()));
        std::string cwd_color   = getIniValue(theme, "colors", "path");

        std::string user        = trim(sysexec("whoami"));
        std::string user_color  = getIniValue(theme, "colors", "username");

        std::string hostname        = trim(sysexec("cat /proc/sys/kernel/hostname"));
        std::string hostname_color  = getIniValue(theme, "colors", "hostname");;

        std::string cmd = "";

        char **args;

        std::string prompt = parsePromptFormat(getIniValue(theme, "prompt", "format"), prompt_character, cwd, cwd_color,
                                                user, user_color, hostname, hostname_color);

        std::cout << prompt;

        std::getline(std::cin, cmd);
        if (std::cin.eof()) {
            exit(0);
        }
        std::vector<std::string> tokens = split(&cmd);
        if (tokens.empty()) {
            continue;
        }

        std::vector<const char *> cstringTokens = cstringArray(tokens);
        cmd = cstringTokens[0];
        args = const_cast<char **>(cstringTokens.data());

        for (int i = 0; args[i] != nullptr; ++i) {
            std::string modifiedArgument = std::string(args[i]);
            modifiedArgument = replaceSubstring(modifiedArgument, "~", homePath());
            char* modifiedArgumentCstr = new char[modifiedArgument.size() + 1];
            std::strcpy(modifiedArgumentCstr, modifiedArgument.c_str());

            args[i] = modifiedArgumentCstr;
        }

        // FIXME: are some of these vectors redundant
        // FIXME: add support for quotes and escaping
        std::vector<char *> command;
        for (char **arg = args; *arg != nullptr; ++arg) {
            if (strcmp(*arg, "||") == 0) {
                command.push_back(nullptr);
                if (command.at(0) == nullptr){
                    printf("%s: Syntax error\n", *argv);
                    command.clear();
                    break;
                }
                if (runCommand(command.data()) != 0) {
                    command.clear();
                    continue;
                }
                command.clear();
                break;
            } else if (strcmp(*arg, ";") == 0) {
                command.push_back(nullptr);
                if (command.at(0) == nullptr){
                    //there is no error with just a semicolon
                    command.clear();
                    break;
                }
                runCommand(command.data());
                command.clear();
                continue;
            } else if (strcmp(*arg, "&&") == 0) {
                command.push_back(nullptr);
                if (command.at(0) == nullptr){
                    printf("%s: Syntax error\n", *argv);
                    command.clear();
                    break;
                }
                if (runCommand(command.data()) == 0) {
                    command.clear();
                    continue;
                }
                command.clear();
                break;
            } else {
                command.push_back(*arg);
            }
        }
        // run command if no thingiers were found
        if (command.size() > 0 && command.at(0)!=nullptr) {
            command.push_back(nullptr);
            runCommand(command.data());
        }
    }

    return 0;
}