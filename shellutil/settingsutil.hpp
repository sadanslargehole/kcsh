#ifndef KCSH_SETTINGSUTIL
#define KCSH_SETTINGSUTIL

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "stringutil.hpp"

typedef std::string string;

using IniData = std::map<string, std::map<string, string>>;

inline IniData parseIniFile(const string& filename) {
    IniData iniData;
    string currentSection;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return iniData;
    }

    string line;
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
            if (equalsPos != string::npos) {
                string key = line.substr(0, equalsPos);
                string value = line.substr(equalsPos + 1);
                iniData[currentSection][key] = value;
            }
        }
    }

    file.close();
    return iniData;
}

#endif