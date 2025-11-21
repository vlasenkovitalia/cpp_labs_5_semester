#include <iostream>
#include <string>

int main() {
    std::cout << "Calculator - Type 'quit' to exit\n";
    
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "quit" || line == "exit") break;
        if (line.empty()) continue;
        
        std::cout << "Basic calculator - will evaluate: " << line << "\n";
    }
    
    return 0;
}