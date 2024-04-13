#include "shellutil/colors.hpp"
#include "shellutil/pluginloader.hpp"
#include "shellutil/settingsutil.hpp"
#include "shellutil/shellexec.hpp"
#include "shellutil/stringutil.hpp"
#include "shellutil/sysutil.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits.h>
#include <regex>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
namespace fs = std::filesystem;
std::unordered_map<std::string, std::string> shellVars;
fs::path settingsDir = homePath().string() + "/.config/kcsh/";
fs::path themesDir = settingsDir.string() + "/themes/";
extern std::vector<std::pair<std::string, Replacement>>
    promptReplacementMapping;

int main([[gnu::unused]] int argc,[[gnu::unused]] char **argv) {

  setenv("OLDPWD", fs::current_path().c_str(), 1);
  setenv("PWD", fs::current_path().c_str(), 1);

  IniData settings;
  IniData theme;

  fs::path pluginsPath = settingsDir.string() + "/plugins";

  fs::create_directory(homePath().string() + "/.config");
  fs::create_directory(settingsDir);
  fs::create_directory(themesDir);
  fs::create_directory(pluginsPath);

  PluginLoader pluginLoader;
  for (const auto &entry : fs::directory_iterator(pluginsPath)) {
    if (entry.path().filename().extension() == ".so") {
#ifdef NDEBUG
      std::cout << "Loading plugin " << entry.path().string() << std::endl;
#endif
      pluginLoader.loadPlugin(entry.path());
    }
  }

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

  // Set $SHELL
  char srlbuffer[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", srlbuffer, sizeof(srlbuffer) - 1);
  if (len != -1) {
    srlbuffer[len] = '\0';
    setenv("SHELL", srlbuffer, 1);
  } else {
    std::cerr << "Not setting $SHELL - could not get absolute path";
  }

  while (true) {

    std::string prompt_color = FG_WHITE;
    std::string prompt_character =
        getIniValue(theme, "prompt", "promptcharacter") + " ";

    std::string cwd = prettyPath(trim(fs::current_path()));
    std::string cwd_color = getIniValue(theme, "colors", "path");

    std::string user = trim(sysexec("whoami"));
    std::string user_color = getIniValue(theme, "colors", "username");

    std::string hostname = trim(sysexec("cat /proc/sys/kernel/hostname"));
    std::string hostname_color = getIniValue(theme, "colors", "hostname");
    ;

    std::string cmd = "";

    char **args;

    std::string prompt = parsePromptFormat(
        getIniValue(theme, "prompt", "format"), prompt_character, cwd,
        cwd_color, user, user_color, hostname, hostname_color, pluginLoader);

    std::cout << prompt;

    std::getline(std::cin, cmd);
    if (std::cin.eof()) {
      exit(0);
    }

    // for (int i = 0; args[i] != nullptr; ++i) {
    //     std::string modifiedArgument = std::string(args[i]);
    //     modifiedArgument = replaceSubstring(modifiedArgument, "~",
    //     homePath()); modifiedArgument =
    //     replaceEnvironmentVariables(modifiedArgument); char*
    //     modifiedArgumentCstr = new char[modifiedArgument.size() + 1];
    //     std::strcpy(modifiedArgumentCstr, modifiedArgument.c_str());
    //
    //     args[i] = modifiedArgumentCstr;
    // }
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
    for (char *c = cmd.data(); *c != 0x0; c++) {
      if (envQuote) {
        envValue += *c;
        if (*(c + 2) == 0x0) {
          if (*(c + 1) != '"')
            envValue += *(c + 1);
          isSessionVar = true;
          break;
        } else if (*(c + 1) == '"') {
          inEnv = false;
          envQuote = false;
          // why
          c++;
          while (*(c + 1) == ' ') {
            c++;
          }
        }
        continue;

      } else if (inEnv) {
        if (!escape) {
          if (*c == '\\') {
            escape = true;
            continue;
          }
          if (*c == ';') {
            soFar.clear();
            setSessionVar(envName, envValue);
            canEnv = true;
            inEnv = false;
            continue;
          }
          if (*c == '&' && *(c + 1) == '&') {
            c++;
            soFar.clear();
            setSessionVar(envName, envValue);
            canEnv = true;
            inEnv = false;
            continue;
          }
          if (*c == '|' && *(c + 1) == '|') {
            c++;
            soFar.clear();
            setSessionVar(envName, envValue);
            canEnv = true;
            inEnv = false;
            continue;
          }
        } else
          escape = false;
        envValue += *c;
        if (*(c + 1) == 0x0) {
          isSessionVar = true;
          break;
        } else if (*(c + 1) == ' ') {
          inEnv = false;
          while (*(c + 1) == ' ') {
            c++;
          }
          continue;
        }
      }
      // END ENV
      if (*c == '\\') {
        if (escape) {
          soFar += '\\';
        }
        escape = !escape;
        continue;
      }
      if (canEnv && *c == '=' && !escape) {
        // TODO: add envName validation
        envName.clear();
        envValue.clear();
        envName = soFar;
        soFar.clear();
        inEnv = true;
        if (*(c + 1) == '"') {
          envQuote = true;
          c++;
        }
        // dont mistake another equals sign somewhere else for another env var
        canEnv = false;
        continue;
      }
      if (*c == '$' && !escape) {
        c++;
        // Check if the
        //  TODO: dont use regex for this matching
        //  FIXME: this is wildly broken
        //  WTF is this
        if (!std::regex_match(c, std::regex("^[a-zA-Z_].*"))) {
          soFar += '$';
          c--;
          continue;
        }
        std::string sVar = "";
        while (*c != 0x0 && *c != ' ') {
          if (!std::regex_match(c, std::regex("^[a-zA-Z0-9_].*"))) {
            break;
          }
          sVar += *c;
          c++;
        }
        c--;
        if (shellVars.contains(sVar)) {
          soFar.append(shellVars[sVar]);
        } else if (std::getenv(sVar.c_str())) {
          auto fromEnv = std::getenv(sVar.c_str());
          soFar.append(fromEnv);
        }
        continue;
      }
      if (*c == ' ' && !soFar.empty()) {
        canEnv = false;
      }
      if (*c == ';' && !escape) {
        runCommand(soFar, &envName, &envValue);
        canEnv = true;
        soFar.clear();
        envName.clear();
        envValue.clear();
        while (*(c + 1) == ' ') {
          c++;
        }
        continue;
      }
      if (*c == '&' && *(c + 1) == '&' && !escape) {
        if (runCommand(soFar, &envName, &envValue) != 0) {
          soFar.clear();
          envName.clear();
          envValue.clear();
          break;
        }
        soFar.clear();
        envName.clear();
        envValue.clear();
        c++;
        while (*(c + 1) == ' ') {
          c++;
        }
        continue;
      }
      if (*c == '|' && *(c + 1) == '|' && !escape) {
        if (runCommand(soFar, &envName, &envValue) == 0) {
          envName.clear();
          envValue.clear();
          soFar.clear();
          break;
        }
        envName.clear();
        envValue.clear();
        soFar.clear();
        c++;
        while (*(c + 1) == ' ') {
          c++;
        }
        continue;
      }
      if (escape){
        escape = false;
      }
      soFar += c;
      // REDIRECTING
      // OVERWRITING
      // PIPE
    }
    runCommand(soFar, nullptr, nullptr);
  }

  return 0;
}
