#pragma once

#include "kcshglobal.hpp"
#include <map>
#include <utility>

class KCSHPlugin {
  public:
    std::vector<std::pair<std::string, Replacement>> promptReplacementMapping;

    virtual ~KCSHPlugin() = default;
    virtual void initialize() = 0;
    void registerPromptReplacement(std::string wildcard, Replacement replaceWith) {
        promptReplacementMapping.push_back(std::make_pair(wildcard, replaceWith));
    }
};