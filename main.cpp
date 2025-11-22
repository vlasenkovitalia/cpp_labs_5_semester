#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <map>

enum class TokenType {
    NUMBER,
    OPERATOR,
    UNARY_OPERATOR,
    LPAREN,
    RPAREN
};

struct Token {
    TokenType type;
    std::string text;
    double value;
};

struct OpInfo {
    int precedence;
    bool right_assoc;
};

std::map<std::string, OpInfo> op_info = {
    {"+", {2, false}}, {"-", {2, false}},
    {"*", {3, false}}, {"/", {3, false}},
    {"^", {4, true}},
    {"+u", {5, false}}, {"-u", {5, false}}  // Unary operators
};

std::vector<Token> tokenize(const std::string& s) {
    std::vector<Token> tokens;
    size_t i = 0;
    bool expect_unary = true;
    
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
            expect_unary = false;
            continue;
        }
        
        if (s[i] == '(') {
            tokens.push_back({TokenType::LPAREN, "("});
            ++i;
            expect_unary = true;
        }
        else if (s[i] == ')') {
            tokens.push_back({TokenType::RPAREN, ")"});
            ++i;
            expect_unary = false;
        }
        else if (s[i] == '+' || s[i] == '-' || s[i] == '*' || s[i] == '/' || s[i] == '^') {
            if (expect_unary && (s[i] == '+' || s[i] == '-')) {
                tokens.push_back({TokenType::UNARY_OPERATOR, std::string(1, s[i]) + "u"});
            } else {
                tokens.push_back({TokenType::OPERATOR, std::string(1, s[i])});
            }
            ++i;
            expect_unary = true;
        }
        else {
            throw std::runtime_error("Unknown character: " + std::string(1, s[i]));
        }
    }
    
    return tokens;
}

std::vector<Token> shunting_yard(const std::vector<Token>& tokens) {
    std::vector<Token> output;
    std::vector<Token> opstack;
    
    for (const auto& token : tokens) {
        if (token.type == TokenType::NUMBER) {
            output.push_back(token);
        }
        else if (token.type == TokenType::OPERATOR || token.type == TokenType::UNARY_OPERATOR) {
            while (!opstack.empty() && 
                   (opstack.back().type == TokenType::OPERATOR || 
                    opstack.back().type == TokenType::UNARY_OPERATOR)) {
                const auto& top_op = op_info[opstack.back().text];
                const auto& curr_op = op_info[token.text];
                
                if ((curr_op.right_assoc && curr_op.precedence < top_op.precedence) ||
                    (!curr_op.right_assoc && curr_op.precedence <= top_op.precedence)) {
                    output.push_back(opstack.back());
                    opstack.pop_back();
                } else {
                    break;
                }
            }
            opstack.push_back(token);
        }
        else if (token.type == TokenType::LPAREN) {
            opstack.push_back(token);
        }
        else if (token.type == TokenType::RPAREN) {
            while (!opstack.empty() && opstack.back().type != TokenType::LPAREN) {
                output.push_back(opstack.back());
                opstack.pop_back();
            }
            if (opstack.empty()) throw std::runtime_error("Mismatched parentheses");
            opstack.pop_back(); // Remove LPAREN
        }
    }
    
    while (!opstack.empty()) {
        if (opstack.back().type == TokenType::LPAREN) {
            throw std::runtime_error("Mismatched parentheses");
        }
        output.push_back(opstack.back());
        opstack.pop_back();
    }
    
    return output;
}

double evaluate_rpn(const std::vector<Token>& rpn) {
    std::vector<double> stack;
    
    for (const auto& token : rpn) {
        if (token.type == TokenType::NUMBER) {
            stack.push_back(token.value);
        }
        else if (token.type == TokenType::OPERATOR) {
            if (stack.size() < 2) throw std::runtime_error("Not enough operands");
            double b = stack.back(); stack.pop_back();
            double a = stack.back(); stack.pop_back();
            
            if (token.text == "+") stack.push_back(a + b);
            else if (token.text == "-") stack.push_back(a - b);
            else if (token.text == "*") stack.push_back(a * b);
            else if (token.text == "/") {
                if (b == 0) throw std::runtime_error("Division by zero");
                stack.push_back(a / b);
            }
            else if (token.text == "^") stack.push_back(std::pow(a, b));
        }
        else if (token.type == TokenType::UNARY_OPERATOR) {
            if (stack.empty()) throw std::runtime_error("Not enough operands");
            double a = stack.back(); stack.pop_back();

            if (token.text == "+u") stack.push_back(a);
            else if (token.text == "-u") stack.push_back(-a);
        }
    }
    
    if (stack.size() != 1) throw std::runtime_error("Invalid expression");
    return stack.back();
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
            auto rpn = shunting_yard(tokens);
            double result = evaluate_rpn(rpn);
            std::cout << "= " << result << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    return 0;
}