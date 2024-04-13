#ifndef KCSH_SETTINGSUTIL
#define KCSH_SETTINGSUTIL

#include "../api/kcshplugin.hpp"
#include "colors.hpp"
#include "pluginloader.hpp"
#include "stringutil.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <regex>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

typedef std::string string;

using IniData = std::map<std::string, std::map<std::string, std::pair<std::string, std::string>>>;
using json = nlohmann::json;
namespace fs = std::filesystem;
static std::unordered_map<std::string, std::string> colorMappings({
    {"black", FG_BLACK},
    {"red", FG_RED},
    {"green", FG_GREEN},
    {"yellow", FG_YELLOW},
    {"blue", FG_BLUE},
    {"magenta", FG_MAGENTA},
    {"cyan", FG_CYAN},
    {"white", FG_WHITE},
    {"bg_black", BG_BLACK},
    {"bg_red", BG_RED},
    {"bg_green", BG_GREEN},
    {"bg_yellow", BG_YELLOW},
    {"bg_blue", BG_BLUE},
    {"bg_magenta", BG_MAGENTA},
    {"bg_cyan", BG_CYAN},
    {"bg_white", BG_WHITE},
});
extern fs::path settingsDir;
extern fs::path themesDir;

inline std::string escapeToReadable(const std::string &input) {
    std::ostringstream result;
    for (char c : input) {
        bool inComment = false;
        if (c == ';') {
            inComment = true;
        } else if (c == '\n') {
            inComment = false;
        }

        if (c == '\033') {
            result << "\\e";
        } else if (c == ';' && !inComment) {
            result << "\\;";
        } else {
            result << c;
        }
    }
    return result.str();
}

inline std::string readableToEscape(const std::string &input) {
    std::ostringstream result;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\\' && input[i + 1] == 'e') {
            result << '\033';
            i += 2;
        } else if (input[i] == '\\' && input[i + 1] == ';') {
            result << ';';
            i += 2;
        } else {
            result << input[i];
            i++;
        }
    }
    return result.str();
}

inline IniData parseIniFile(const std::string &filename) {
    IniData iniData;
    std::string currentSection;

    std::ifstream file(filename);
    if (!file.is_open()) {
#ifdef NDEBUG
        std::cerr << "Error opening file: " << filename << std::endl;
#endif
        return iniData;
    }

    std::string line;
    while (getline(file, line)) {
        line = trim(line);
        line.erase(line.find_last_not_of(" \t") + 1);
        line.erase(0, line.find_first_not_of(" \t"));

        if (line.empty() || line[0] == ';') {
            continue;
        }

        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = readableToEscape(line.substr(1, line.length() - 2));
        } else {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = readableToEscape(line.substr(0, equalsPos));
                std::string value = readableToEscape(line.substr(equalsPos + 1));
                iniData[currentSection][key] = std::make_pair(value, "");
            }
        }
    }

    file.close();
    return iniData;
}
inline json getDefaultConfigJson() {
    throw "NOT IMPLIMENTED";
    return nullptr;
}
inline IniData getDefaultConfig() {
    IniData defaultConfig;

    defaultConfig["appearance"]["theme"] = std::make_pair("default", "run kcshthemes to see what themes are available");

    return defaultConfig;
}
inline json getDefaulThemeJson() {
    throw "NOT IMPLIMENTED";
    return nullptr;
}
inline IniData getDefaultTheme() {
    IniData defaultTheme;

    defaultTheme["info"]["name"] = std::make_pair("Default", "theme name");
    defaultTheme["prompt"]["promptcharacter"] = std::make_pair("$", "");
    defaultTheme["prompt"]["format"] =
        std::make_pair("%BOLD%[%COLOREDUSER%%RESET%%BOLD%@%COLOREDHOST%%RESET%%BOLD%]%RESET% "
                       "%COLOREDPATH% %RESET%%NEWLINE%%PROMPTCHARACTER%",
                       "Prompt format. Available placeholders: \n\
; %COLOREDUSER% and %USER% for the formatted and unformatted user, respectively\n\
; %COLOREDHOST% and %HOST% for the formatted and unformatted host, respectively\n\
; %COLOREDPATH% and %PATH% for the formatted and unformatted path, respectively\n\
; %NEWLINE% will be substituted with a newline character\n\
; %PROMPTCHARACTER% is replaced with the promptcharacter configuration key plus a space\n\
; %BOLD%  will make following text bold\n\
; %ULINE% will make following text underlined\n\
; %RESET% will reset text formatting\n");

    defaultTheme["colors"]["path"] = std::make_pair(FG_BLUE, "color code/s for path \n; see "
                                                             "https://keli5.gthub.io/kcsh-resources/"
                                                             "colors.html for help with color codes");
    defaultTheme["colors"]["hostname"] = std::make_pair(FG_MAGENTA, "color code/s for hostname");
    defaultTheme["colors"]["username"] = std::make_pair(FG_RED, "color code/s for username");

    return defaultTheme;
}
inline void saveJsonFile(const json &data, const std::string &filename) {
    throw "NOT IMPLIMENTED";
    return;
}
inline void saveIniFile(const IniData &iniData, const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    for (const auto &section : iniData) {
        file << "[" << section.first << "]\n";
        for (const auto &pair : section.second) {
            file << pair.first << "=" << escapeToReadable(pair.second.first);

            if (!pair.second.second.empty()) {
                file << " ; " << escapeToReadable(pair.second.second); // if there's a comment add it
            }

            file << "\n";
        }
        file << "\n";
    }

    file.close();
}

// TODO: Is this slow?
inline std::string getIniValue(const IniData &iniData, const std::string &section, const std::string &key) {
    auto sectionIter = iniData.find(section);
    if (sectionIter != iniData.end()) {
        auto keyIter = sectionIter->second.find(key);
        if (keyIter != sectionIter->second.end()) {
            std::size_t commentPos = keyIter->second.first.find(';');
            if (commentPos != std::string::npos) {
                return readableToEscape(trim(keyIter->second.first.substr(0, commentPos)));
            } else {
                return readableToEscape(trim(keyIter->second.first));
            }
        }
    }
    return "";
}

inline std::string parseColorBG(std::string color) {
    if (colorMappings.contains("bg_" + color)) {
        return colorMappings["bg_" + color];
    } else {
#ifdef NDEBUG
        std::cerr << "background color: "
                  << "bg_" + color << " not found"
                  << "\n";
#endif
        return "";
    }
}
inline std::string parseColor(std::string color) {
    if (colorMappings.contains(color)) {
        return colorMappings[color];
    } else {
#ifdef NDEBUG
        std::cerr << "foreground color: " << color << " not found"
                  << "\n";
#endif
        return "";
    }
}
inline std::string parseSection(json part, std::string data) {
    std::string toRet = "";
    if (part["prefix"]["content"] != nullptr) {
        auto x = part["prefix"];
        toRet += parseColor(x["foreground"]);
        toRet += parseColorBG(x["background"]);
        toRet += x["content"];
        toRet += RESET;
    }
    toRet += parseColor(part["foreground"]);
    toRet += parseColor(part["background"]);
    toRet += data;
    toRet += RESET;
    if (part["postfix"]["content"] != nullptr) {
        auto x = part["postfix"];
        toRet += parseColor(x["foreground"]);
        toRet += parseColorBG(x["background"]);
        toRet += x["content"];
        toRet += RESET;
    }
    return toRet;
}
inline std::string parsePromptFormat(json config,  std::string user, string host, std::string cwd, PluginLoader loader) {
    // hell v2:tm:
    // parse username
    auto p = config["prompt"];
    std::string toRet = "";
    toRet += parseSection(p["username"], user);
    toRet += parseSection(p["hostname"], host);
    toRet += parseSection(p["path"], cwd);
    toRet += '\n';
    toRet += p["promptChar"];
    // wow this is like hell actually
    // std::string promptOutput = ptemplate;
    // // hard code to prevent plugins from fiddling with it and perform them first
    // promptOutput = replaceSubstring(promptOutput, "%COLOREDUSER%", user_color + user);
    // promptOutput = replaceSubstring(promptOutput, "%USER%", user);
    // promptOutput = replaceSubstring(promptOutput, "%COLOREDHOST%", host_color + host);
    // promptOutput = replaceSubstring(promptOutput, "%HOST%", host);
    // promptOutput = replaceSubstring(promptOutput, "%COLOREDPATH%", cwd_color + cwd);
    // promptOutput = replaceSubstring(promptOutput, "%PATH%", cwd);
    // promptOutput = replaceSubstring(promptOutput, "%NEWLINE%", "\n");
    // promptOutput = replaceSubstring(promptOutput, "%PROMPTCHARACTER%", prompt_character);
    // promptOutput = replaceSubstring(promptOutput, "%RESET%", RESET);
    // promptOutput = replaceSubstring(promptOutput, "%BOLD%", BOLD);
    // promptOutput = replaceSubstring(promptOutput, "%ULINE%", ULINE);
    // and now everything else
    // FIXME: add plugin support
    // TODO: add plugin support
    // idea: plugin will fiddle the json before we do
    // for (const auto &pluginPair : loader.getPlugins()) {
    //     KCSHPlugin *plugin = pluginPair.second;
    //
    //     for (const auto &replacementPair : plugin->promptReplacementMapping) {
    //         std::string result;
    //         std::visit(
    //             [&result](const auto &value) {
    //                 if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>) {
    //                     result = value;
    //                 } else if constexpr (std::is_invocable_r_v<std::string, decltype(value)>) {
    //                     result = value();
    //                 }
    //             },
    //             replacementPair.second);
    //
    //         promptOutput = replaceSubstring(promptOutput, replacementPair.first, result);
    //     }
    // }

    return toRet;
}

#endif
