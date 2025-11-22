#include <iostream>
#include <string>
#include <stdexcept>
#include "calculator.h"
#include "plugin_system.h"

int main() {
    std::cout << "Calculator\n";

    load_plugins_from_folder("./plugins");

    std::cout << "Type expressions, or 'quit' to exit.\n";
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;
        try {
            auto toks = tokenize(line);
            auto rpn = shunting_yard(toks);
            double v = evaluate_rpn(rpn);
            std::cout << v << "\n";
        }
        catch (const std::exception& ex) {
            std::cout << "Error: " << ex.what() << "\n";
        }
        catch (...) {
            std::cout << "Unknown error occurred\n";
        }
    }
    
    cleanup_plugins();
    return 0;
}
