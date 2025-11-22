#include <iostream>
#include <string>
#include "calculator.h"
#include "plugin_system.h"


int main() {
    std::cout << "Calculator - Type 'quit' to exit\n";

    register_builtin_functions();
    
    load_plugins_from_folder("./plugins");
    
    print_available_functions();

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "quit" || line == "exit") break;
        if (line.empty()) continue;
        
        try {
            auto tokens = tokenize(line);
            auto rpn = shunting_yard(tokens);
            double result = evaluate_rpn(rpn);
            std::cout << "= " << result << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    for (HMODULE handle : g_loaded_modules) {
        FreeLibrary(handle);
    }

    return 0;
}