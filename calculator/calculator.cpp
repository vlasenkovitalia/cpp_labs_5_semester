#include "calculator.h"
#include "plugin_system.h"
#include <cctype>
#include <cmath>
#include <stdexcept>

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
            tokens.push_back({TokenTypeEnum::NUMBER, num_str, std::stod(num_str)});
            expect_unary = false;
            continue;
        }
        if (isalpha(s[i]) || s[i] == '_') {
            size_t start = i;
            while (i < s.size() && (isalnum(s[i]) || s[i] == '_')) {
                ++i;
            }
            std::string ident = s.substr(start, i - start);
            tokens.push_back({TokenTypeEnum::IDENTIFIER, ident});
            expect_unary = false;
            continue;
        }
        if (s[i] == '(') {
            tokens.push_back({TokenTypeEnum::LPAREN, "("});
            ++i;
            expect_unary = true;
        }
        else if (s[i] == ')') {
            tokens.push_back({TokenTypeEnum::RPAREN, ")"});
            ++i;
            expect_unary = false;
        }
        else if (s[i] == ',') {
            tokens.push_back({TokenTypeEnum::COMMA, ","});
            ++i;
            expect_unary = true;
        }
        else if (s[i] == '+' || s[i] == '-' || s[i] == '*' || s[i] == '/' || s[i] == '^') {
            if (expect_unary && (s[i] == '+' || s[i] == '-')) {
                tokens.push_back({TokenTypeEnum::UNARY_OPERATOR, std::string(1, s[i]) + "u"});
            } else {
                tokens.push_back({TokenTypeEnum::OPERATOR, std::string(1, s[i])});
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

std::vector<RPNItem> shunting_yard(const std::vector<Token>& tokens) {
    std::vector<RPNItem> output;
    std::vector<Token> opstack;
    std::vector<int> arg_count_stack;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& t = tokens[i];
        if (t.type == TokenTypeEnum::NUMBER) { 
            output.push_back({ t, -1 });
        }
        else if (t.type == TokenTypeEnum::IDENTIFIER) {  
            bool isFunc = false;
            if (i + 1 < tokens.size() && tokens[i + 1].type == TokenTypeEnum::LPAREN) isFunc = true;
            if (isFunc) {
                opstack.push_back(t);
                arg_count_stack.push_back(1); // Start with 1 argument
            }
            else {
                throw std::runtime_error("Unknown identifier without (): " + t.text);
            }
        }
        else if (t.type == TokenTypeEnum::COMMA) {   
            if (opstack.empty()) throw std::runtime_error("Misplaced comma");
            while (!opstack.empty() && opstack.back().type != TokenTypeEnum::LPAREN) {  
                output.push_back({ opstack.back(), -1 });
                opstack.pop_back();
            }
            if (opstack.empty()) throw std::runtime_error("Misplaced comma / missing '('");
            if (arg_count_stack.empty()) throw std::runtime_error("Comma outside function");
            arg_count_stack.back() += 1; // Increment argument count
        }
        else if (t.type == TokenTypeEnum::OPERATOR || t.type == TokenTypeEnum::UNARY_OPERATOR) {  
            std::string o1 = t.text;
            while (!opstack.empty() && 
                   (opstack.back().type == TokenTypeEnum::OPERATOR || opstack.back().type == TokenTypeEnum::UNARY_OPERATOR)) {  
                std::string o2 = opstack.back().text;
                OpInfo& p1 = op_info[o1];
                OpInfo& p2 = op_info[o2];
                if ((p1.right_assoc && p1.precedence < p2.precedence) || (!p1.right_assoc && p1.precedence <= p2.precedence)) { 
                    output.push_back({ opstack.back(), -1 });
                    opstack.pop_back();
                }
                else break;
            }
            opstack.push_back(t);
        }
        else if (t.type == TokenTypeEnum::LPAREN) {
            opstack.push_back(t);
        }
        else if (t.type == TokenTypeEnum::RPAREN) { 
            bool foundLP = false;
            while (!opstack.empty()) {
                Token top = opstack.back(); opstack.pop_back();
                if (top.type == TokenTypeEnum::LPAREN) { foundLP = true; break; }  
                output.push_back({ top, -1 });
            }
            if (!foundLP) throw std::runtime_error("Mismatched parentheses");
            
            // Handle function call
            if (!opstack.empty() && opstack.back().type == TokenTypeEnum::IDENTIFIER) {  
                Token funcTok = opstack.back(); opstack.pop_back();
                int arg_count = 0;
                if (!arg_count_stack.empty()) {
                    arg_count = arg_count_stack.back();
                    arg_count_stack.pop_back();
                }
                output.push_back({ funcTok, arg_count });
            }
        }
    }
    
    while (!opstack.empty()) {
        Token t = opstack.back(); opstack.pop_back();
        if (t.type == TokenTypeEnum::LPAREN) throw std::runtime_error("Mismatched parentheses");
        output.push_back({ t, -1 });
    }
    
    return output;
}

double evaluate_rpn(const std::vector<RPNItem>& rpn) {
    std::vector<double> stack;
    
    for (const auto& item : rpn) {
        const Token& tk = item.token;
        
        if (tk.type == TokenTypeEnum::NUMBER) {  
            stack.push_back(tk.value);
        }
        else if (tk.type == TokenTypeEnum::OPERATOR) {  
            if (stack.size() < 2) throw std::runtime_error("Insufficient operands for operator " + tk.text);
            double b = stack.back(); stack.pop_back();
            double a = stack.back(); stack.pop_back();
            double r = 0;
            if (tk.text == "+") r = a + b;
            else if (tk.text == "-") r = a - b;
            else if (tk.text == "*") r = a * b;
            else if (tk.text == "/") {
                if (b == 0.0) throw std::runtime_error("Division by zero");
                r = a / b;
            }
            else if (tk.text == "^") {
                r = std::pow(a, b);
            }
            else throw std::runtime_error("Unknown operator " + tk.text);
            stack.push_back(r);
        }
        else if (tk.type == TokenTypeEnum::UNARY_OPERATOR) {
            if (stack.size() < 1) throw std::runtime_error("Insufficient operands for unary operator " + tk.text);
            double a = stack.back(); stack.pop_back();
            if (tk.text == "+u") stack.push_back(a);
            else if (tk.text == "-u") stack.push_back(-a);
            else throw std::runtime_error("Unknown unary operator " + tk.text);
        }
        else if (tk.type == TokenTypeEnum::IDENTIFIER) {  
            int argcount = item.arg_count;
            if (argcount < 0) throw std::runtime_error("Internal error: function argcount missing");
            if ((int)stack.size() < argcount) throw std::runtime_error("Not enough values for function " + tk.text);
            
            std::vector<double> args(argcount);
            for (int k = argcount - 1; k >= 0; --k) {
                args[k] = stack.back(); stack.pop_back();
            }
            
            auto itf = g_functions.find(tk.text);
            if (itf == g_functions.end())
                throw std::runtime_error("Unknown function: " + tk.text);
                
            double res = call_function(itf->second, args);
            stack.push_back(res);
        }
    }
    
    if (stack.size() != 1) throw std::runtime_error("Malformed expression (stack size != 1 after eval)");
    return stack.back();
}
