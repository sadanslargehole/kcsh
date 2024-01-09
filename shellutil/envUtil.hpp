#ifndef KCSH_ENVUTIL
#define KCSH_ENVUTIL
#include "shellexec.hpp"
#include <string>
extern std::unordered_map<std::string, std::string> shellVars;
inline void setSessionVar(std::string key, std::string val){
        if (std::getenv(key.c_str()) != NULL) {
            setenv(key.c_str(), val.c_str(), 1);

        } else
            shellVars[key] = val;
}
#endif