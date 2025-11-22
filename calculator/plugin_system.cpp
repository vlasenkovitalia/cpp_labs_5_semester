#include "plugin_system.h"
#include <iostream>
#include <stdexcept>

std::map<std::string, FunctionEntry> g_functions;
std::vector<HMODULE> g_loaded_modules;

void load_plugins_from_folder(const std::string& folder) {
    std::string search_path = folder + "/*.dll";
    
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle = FindFirstFileA(search_path.c_str(), &find_data);
    
    if (find_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "No plugins folder or no DLLs found in " << folder << "\n";
        return;
    }
    
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string dll_path = folder + "/" + find_data.cFileName;
            try_load_plugin(dll_path);
        }
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
}

double call_function(const FunctionEntry& fe, const std::vector<double>& args) {
    if (fe.fn == nullptr) throw std::runtime_error("Function implementation missing for " + fe.name);
    
    // Check arity
    if ((fe.min_arity >= 0 && (int)args.size() < fe.min_arity) || 
        (fe.max_arity >= 0 && (int)args.size() > fe.max_arity)) {
        throw std::runtime_error("Function " + fe.name + " called with wrong number of arguments");
    }

    try {
        double result = fe.fn(args.empty() ? nullptr : &args[0], (int)args.size());
        return result;
    }
    catch (const std::exception& ex) {
        throw std::runtime_error(std::string("Function ") + fe.name + " threw: " + ex.what());
    }
    catch (...) {
        throw std::runtime_error(std::string("Function ") + fe.name + " threw unknown exception");
    }
}

bool try_load_plugin(const std::string& dll_path) {
    HMODULE handle = LoadLibraryA(dll_path.c_str());
    if (!handle) {
        std::cerr << "Failed to load " << dll_path << "\n";
        return false;
    }
    
    auto register_func = (bool(*)(PluginDescriptor*, char*, int))GetProcAddress(handle, "RegisterPlugin");
    if (!register_func) {
        std::cerr << "No RegisterPlugin in " << dll_path << "\n";
        FreeLibrary(handle);
        return false;
    }
    
    PluginDescriptor desc;
    char error_msg[256] = {0};
    bool success = register_func(&desc, error_msg, sizeof(error_msg));
    
    if (!success) {
        std::cerr << "Plugin registration failed: " << error_msg << "\n";
        FreeLibrary(handle);
        return false;
    }
    
    if (!desc.name || !desc.fn) {
        std::cerr << "Invalid plugin descriptor\n";
        FreeLibrary(handle);
        return false;
    }
    
    std::string func_name = desc.name;
    for (char& c : func_name) c = std::tolower(c);
    
    if (g_functions.count(func_name)) {
        std::cerr << "Function " << func_name << " already registered\n";
        FreeLibrary(handle);
        return false;
    }
    
    g_functions[func_name] = {func_name, desc.min_arity, desc.max_arity, desc.fn};
    g_loaded_modules.push_back(handle);
    
    std::cout << "Loaded plugin: " << func_name << " from " << dll_path << "\n";
    return true;
}

void cleanup_plugins() {
    for (HMODULE h : g_loaded_modules) {
        FreeLibrary(h);
    }
    g_loaded_modules.clear();
}
