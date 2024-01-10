#include "shellutil/colors.hpp"
#include "shellutil/envUtil.hpp"
#include "shellutil/settingsutil.hpp"
#include "shellutil/shellexec.hpp"
#include "shellutil/stringutil.hpp"
#include "shellutil/sysutil.hpp"
#include "shellutil/vecutil.hpp"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

namespace fs = std::filesystem;
fs::path settingsDir = homePath().string() + "/.config/kcsh/";
fs::path themesDir = settingsDir.string() + "/themes/";
std::unordered_map<std::string, std::string> shellVars;
int lastStatus = 0;
// NOLINTNEXTLINE
int main([[gnu::unused]] int argc, char **argv) {
  setenv("OLDPWD", fs::current_path().c_str(), 1);
  setenv("PWD", fs::current_path().c_str(), 1);

  fs::path settingsDir = homePath().concat("/.config/kcsh");
  IniData settings;
  IniData theme;

  fs::create_directory(homePath().string() + "/.config");
  fs::create_directory(settingsDir);
  fs::create_directory(themesDir);

  settings = parseIniFile(settingsDir.string() + "/kcsh_config.ini");

  if (settings.empty()) {
#ifdef NDEBUG
    std::cout << "Creating default config in " + settingsDir.string()
              << std::endl;
#endif
    settings = getDefaultConfig();

    saveIniFile(settings, settingsDir.string() + "/kcsh_config.ini");
  }

  fs::path themePath = themesDir.string() + "/" +
                       getIniValue(settings, "appearance", "theme") + ".ini";
  theme = parseIniFile(themePath);
  setenv("KCSH_THEME",
         replaceSubstring(themePath.filename(), ".ini", "").c_str(), true);

  if (theme.empty()) {
#ifdef NDEBUG
    std::cout << "Creating " + getIniValue(settings, "appearance", "theme") +
                     " theme in " + themesDir.string()
              << std::endl;
#endif
    theme = getDefaultTheme();
    saveIniFile(theme, themePath);
  }

  while (true) {
std::string prompt_color;
    if (lastStatus!=0) {
       prompt_color = FG_RED;
    } else {
      prompt_color = FG_WHITE;
    }
    std::string prompt_character =
        prompt_color + getIniValue(theme, "prompt", "promptcharacter")+RESET + " ";

    std::string cwd = prettyPath(trim(fs::current_path()));
    std::string cwd_color = getIniValue(theme, "colors", "path");

    std::string user = trim(sysexec("whoami"));
    std::string user_color = getIniValue(theme, "colors", "username");

    std::string hostname = trim(sysexec("cat /proc/sys/kernel/hostname"));
    std::string hostname_color = getIniValue(theme, "colors", "hostname");

    std::string prompt = parsePromptFormat(
        getIniValue(theme, "prompt", "format"), prompt_character, cwd,
        cwd_color, user, user_color, hostname, hostname_color);

    std::cout << prompt;

    std::string cmd = "";

    std::getline(std::cin, cmd);
    if (std::cin.eof()) {
      exit(0);
    }
    if (cmd.empty()) {
      lastStatus = 0;
      continue;
    }
    // all the chars parsed so far
    std::string soFar;
    // the name of the env var(if found)
    std::string envName;
    // value of env var (if found)
    std::string envValue;
    // is the env var a quoted string
    bool envQuote = false;
    // are we parsing an env var
    bool inEnv = false;
    // can we set an env var
    bool canEnv = true;
    bool isSessionVar = false;
    // state for if we are escaped
    bool escape = false;
    for (char *in = cmd.data(); *in != '\0'; in++) {
      if (envQuote) {
        envValue += *in;
        if (*(in + 2) == '\0') {
          isSessionVar = true;
          break;
        } else if (*(in + 1) == '"') {
          inEnv = false;
          envQuote = false;
          in++;
          // fix whitespace messing up command
          while (*(in + 1) == ' ') {
            in++;
          }
        }
        continue;
      } else if (inEnv) {
        if (!escape) {
          if (*in == '\\') {
            escape = true;
            continue;
          }
          if (*in == ';') {
            soFar.clear();
            setSessionVar(envName, envValue);
            inEnv = false;
            canEnv = true;
            continue;
          }

          if (*in == '&' && *(in + 1) == '&') {
            soFar.clear();
            setSessionVar(envName, envValue);
            canEnv = true;
            inEnv = false;
            in++;
            continue;
          }
          if (*in == '|' && *(in + 1) == '|') {
            soFar.clear();
            setSessionVar(envName, envValue);
            canEnv = true;
            inEnv = false;
            in++;
            continue;
          }

        } else
          escape = false;

        envValue += *in;
        // if the next char is null, we need to set the var for the
        // session, not just for the program that might be run
        if (*(in + 1) == '\0') {
          isSessionVar = true;
          break;
        } else if (*(in + 1) == ' ') {
          inEnv = false;
          // fix whitespace messing up command
          while (*(in + 1) == ' ') {
            in++;
          }
          continue;
        }
      }

      if (*in == '\\') {
        if (escape) {
          soFar += '\\';
        }
        escape = !escape;
        continue;
      }
      if (canEnv && *in == '=' && !escape) {
        envName.clear();
        envValue.clear();
        envName = soFar;
        soFar.clear();
        inEnv = true;
        if (*(in + 1) == '"') {
          envQuote = true;
          in++;
        }

        // dont mistake another equals sign somewhere else for an env
        // var
        canEnv = false;
        continue;
      }
      if (*in == '$' && !escape) {
        in++;
        if (!std::regex_match(in, std::regex("^[a-zA-Z_].*"))) {
          soFar += '$';
          in--;
          continue;
        }
        std::string sVar = "";
        while (*in != 0x0 && *in != ' ') {
          if (!std::regex_match(in, std::regex("^[a-zA-Z0-9_].*"))) {
            break;
          }
          sVar += *in;
          in++;
        }
        // this fixes werid behaivor with the loops increment
        in--;
        // make this its own function?

        if (shellVars.contains(sVar)) {
          soFar.append(shellVars[sVar]);
        } else if (std::getenv(sVar.c_str())) {
          auto fromEnv = std::getenv(sVar.c_str());
          soFar.append(fromEnv);
        }
        continue;
      }
      if (*in == ' ' && !(soFar.empty())) {
        // if we encouter a space, we will no longer parse an env value
        canEnv = false;
      }
      if (*in == ';') {
        runCommand(const_cast<char **>(cstringArray(split(soFar)).data()), envName.data(), envValue.data());
        canEnv = true;
        soFar.clear();
        envName.clear();
        envValue.clear();
        // fix whitespace messing up command
        while (*(in + 1) == ' ') {
          in++;
        }
        continue;
      } else if (*in == '&' && *(in + 1) == '&') {
        if (runCommand(const_cast<char **>(cstringArray(split(soFar)).data()), envName.data(),
                       envValue.data()) == 0) {
          envName.clear();
          envValue.clear();
          soFar.clear();
          // increment pointer so we dont add `&` to the command thing
          // to call
          in++;
          // fix whitespace messing up command
          while (*(in + 1) == ' ') {
            in++;
          }
          continue;
        }
        envValue.clear();
        envName.clear();
        soFar.clear();
        break;
      } else if (*in == '|' && *(in + 1) == '|') {
        if (runCommand(const_cast<char **>(cstringArray(split(soFar)).data()), envName.data(),
                       envValue.data()) != 0) {
          envName.clear();
          envValue.clear();
          soFar.clear();
          // increment pointer so we dont add `|` to the command thing
          // to call
          in++;
          // fix whitespace messing up command
          while (*(in + 1) == ' ') {
            in++;
          }
          continue;
        }
        envValue.clear();
        envName.clear();
        soFar.clear();
        break;
      }
      if (escape)
        escape = false;
      soFar += *in;
    }
    // post parseing options
    if (isSessionVar) {
      setSessionVar(envName, envValue);
    } else if (!envName.empty()) {
      lastStatus = runCommand(const_cast<char **>(cstringArray(split(soFar)).data()), envName.data(), envValue.data());
    } else if (!soFar.empty()) {
      lastStatus = runCommand(const_cast<char **>(cstringArray(split(soFar)).data()), (char **)nullptr,
                              (char **)nullptr);
    }
  }

  return 0;
}