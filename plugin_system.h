#pragma once
#include <windows.h>
#include <string>
#include <map>
#include <vector>

extern "C" {
    typedef double (*plugin_fn_t)(const double* args, int nargs);
    
    struct PluginDescriptor {
        const char* name;
        int min_arity;
        int max_arity;
        plugin_fn_t fn;
    };
}

struct FunctionEntry {
    std::string name;
    int min_arity;
    int max_arity;
    plugin_fn_t fn;
    HMODULE owner_module;
};

extern std::map<std::string, FunctionEntry> g_functions;

extern std::vector<HMODULE> g_loaded_modules;

void load_plugins_from_folder(const std::string& folder);
double call_function(const FunctionEntry& fe, const std::vector<double>& args);
bool try_load_plugin(const std::string& dll_path);
void cleanup_plugins();