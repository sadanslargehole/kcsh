// Compile me with ` g++ -shared -fPIC -o example.so example_plugin.cpp ` and put the output in `~/.config/kcsh/plugins`!
// This plugin only compiles against standard libraries, kcshplugin.hpp and kcshglobal.hpp!
#include "kcshplugin.hpp"
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>

class ExamplePlugin : public KCSHPlugin {
    public:
    void initialize() override {
        // Do some initialization stuff
        std::cout << "Hello from example_plugin.cpp!" << std::endl;
        registerPromptReplacement("%TIME%", []() {
            auto time = std::chrono::system_clock::now();
            std::time_t currentTime = std::chrono::system_clock::to_time_t(time);
            return std::string(std::ctime(&currentTime));
        });
        registerPromptReplacement("%EXAMPLE_STRING%", std::string("Hello from example_plugin.cpp! I'm a static string!"));
    }
};

extern "C" KCSHPlugin* createPlugin() {
    return new ExamplePlugin();
}