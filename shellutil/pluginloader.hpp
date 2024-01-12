#pragma once

#include <string>
#include <map>
#include <dlfcn.h>
#include <iostream>
#include <filesystem>

#include "../api/kcshplugin.hpp"

namespace fs = std::filesystem;

class PluginLoader {
private:
    std::map<std::string, KCSHPlugin*> plugins;
public:
    void loadPlugin(const fs::path pluginPath) {
        std::string resolvedPath = pluginPath.string();
        void* handle = dlopen(resolvedPath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "dlopen failed for plugin " + pluginPath.filename().string() + " " + dlerror() << std::endl;
            return;
        }

        using CreatePluginFunc = KCSHPlugin* (*)();
        CreatePluginFunc createPlugin = reinterpret_cast<CreatePluginFunc>(dlsym(handle, "createPlugin"));
        if (!createPlugin) {
            std::cerr << "createPlugin failed for plugin " + pluginPath.filename().string() + " " + dlerror() << std::endl;
            dlclose(handle);
            return;
        }

        KCSHPlugin* plugin = createPlugin();
        plugins[pluginPath.filename().string()] = plugin;

        plugin->initialize();
    }
    const std::map<std::string, KCSHPlugin*>& getPlugins() const {
        return plugins;
    }
};