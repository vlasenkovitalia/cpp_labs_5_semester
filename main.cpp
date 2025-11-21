#include <iostream>
#include <string>
#include <vector>
#include <cctype>

enum class TokenType {
    NUMBER,
    OPERATOR,
    LPAREN,
    RPAREN
};

struct Token {
    TokenType type;
    std::string text;
    double value;
};

std::vector<Token> tokenize(const std::string& s) {
    std::vector<Token> tokens;
    size_t i = 0;
    
    while (i < s.size()) {
        if (isspace(s[i])) {
            ++i;
            continue;
        }
        
        if (isdigit(s[i]) || s[i] == '.') {
            size_t start = i;
            bool has_dot = false;
            while (i < s.size() && (isdigit(s[i]) || (!has_dot && s[i] == '.'))) {
                if (s[i] == '.') has_dot = true;
                ++i;
            }
            std::string num_str = s.substr(start, i - start);
            tokens.push_back({TokenType::NUMBER, num_str, std::stod(num_str)});
            continue;
        }
        
        if (s[i] == '(') {
            tokens.push_back({TokenType::LPAREN, "("});
            ++i;
        }
        else if (s[i] == ')') {
            tokens.push_back({TokenType::RPAREN, ")"});
            ++i;
        }
        else if (s[i] == '+' || s[i] == '-' || s[i] == '*' || s[i] == '/') {
            tokens.push_back({TokenType::OPERATOR, std::string(1, s[i])});
            ++i;
        }
        else {
            throw std::runtime_error("Unknown character: " + std::string(1, s[i]));
        }
    }
    
    return tokens;
}

void print_tokens(const std::vector<Token>& tokens) {
    for (const auto& token : tokens) {
        std::cout << "[" << token.text << "] ";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "Calculator - Type 'quit' to exit\n";
    
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "quit" || line == "exit") break;
        if (line.empty()) continue;
        
        try {
            auto tokens = tokenize(line);
            std::cout << "Tokens: ";
            print_tokens(tokens);
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    return 0;
}