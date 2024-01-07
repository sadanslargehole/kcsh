#ifndef KCSH_SETTINGSUTIL
#define KCSH_SETTINGSUTIL

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <regex>
#include "colors.hpp"
#include "stringutil.hpp"

typedef std::string string;

using IniData = std::map<std::string, std::map<std::string, std::pair<std::string, std::string>>>;

inline std::string escapeToReadable(const std::string& input) {
    std::ostringstream result;
    for (char c : input) {
        if (c == '\033') {
            result << "\\e";
        } else {
            result << c;
        }
    }
    return result.str();
}

inline std::string readableToEscape(const std::string& input) {
    std::ostringstream result;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\\' && input[i + 1] == 'e') {
            result << '\033';
            i += 2;
        } else {
            result << input[i];
            i++;
        }
    }
    return result.str();
}

inline IniData parseIniFile(const std::string& filename) {
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

        // [sections]
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
        } else {
            // parse key=value in sections
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = line.substr(0, equalsPos);
                std::string value = line.substr(equalsPos + 1);
                iniData[currentSection][key] = std::make_pair(value, "");
            }
        }
    }

    file.close();
    return iniData;
}

inline IniData getDefaultConfig() {
    IniData defaultConfig;

    defaultConfig["prompt"]["promptcharacter"] = std::make_pair("$", "");
    defaultConfig["prompt"]["format"] = std::make_pair("[%COLOREDUSER%%RESET%@%COLOREDHOST%%RESET%] %COLOREDPATH% %RESET%%NEWLINE%%PROMPTCHARACTER%",
    "Prompt format. Available placeholders: \n\
; %COLOREDUSER% and %USER% for the formatted and unformatted user, respectively\n\
; %COLOREDHOST% and %HOST% for the formatted and unformatted host, respectively\n\
; %COLOREDPATH% and %PATH% for the formatted and unformatted path, respectively\n\
; %NEWLINE% will be substituted with a newline character\n\
; %RESET% will reset all formatting\n\
; %PROMPTCHARACTER% is replaced with the promptcharacter configuration key plus a space\n\
; See https://keli5.github.io/kcsh-resources/colors.html for help with color codes");

    defaultConfig["colors"]["path"] = std::make_pair(FG_BLUE, "color code/s for path");
    defaultConfig["colors"]["hostname"] = std::make_pair(FG_MAGENTA, "color code/s for hostname");
    defaultConfig["colors"]["username"] = std::make_pair(FG_RED, "color code/s for username");

    return defaultConfig;
}

inline void saveIniFile(const IniData& iniData, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    for (const auto& section : iniData) {
        file << "[" << section.first << "]\n";
        for (const auto& pair : section.second) {
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


//TODO: Is this slow?
inline std::string getIniValue(const IniData& iniData, const std::string& section, const std::string& key) {
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

inline std::string parsePromptFormat(std::string ptemplate, std::string prompt_character,
    std::string cwd, std::string cwd_color,
    std::string user, std::string user_color,
    std::string host, std::string host_color) {
        // wow this is like hell actually
        std::string promptOutput = ptemplate;
        promptOutput = replaceSubstring(promptOutput, "%COLOREDUSER%", user_color + user);
        promptOutput = replaceSubstring(promptOutput, "%USER%", user);
        promptOutput = replaceSubstring(promptOutput, "%COLOREDHOST%", host_color + host);
        promptOutput = replaceSubstring(promptOutput, "%HOST%", host);
        promptOutput = replaceSubstring(promptOutput, "%COLOREDPATH%", cwd_color + cwd);
        promptOutput = replaceSubstring(promptOutput, "%PATH%", cwd);
        promptOutput = replaceSubstring(promptOutput, "%NEWLINE%", "\n");
        promptOutput = replaceSubstring(promptOutput, "%RESET%", RESET);
        promptOutput = replaceSubstring(promptOutput, "%PROMPTCHARACTER%", prompt_character);

        return promptOutput;
}


#endif