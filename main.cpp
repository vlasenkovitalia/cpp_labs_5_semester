#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <cmath>
#include <windows.h>

enum class TokenTypeEnum {
    NUMBER,
    OPERATOR,
    UNARY_OPERATOR,
    LPAREN,
    RPAREN,
    IDENTIFIER,
    COMMA
};

struct Token {
    TokenTypeEnum type;
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

struct RPNItem {
    Token token;
    int arg_count;
};

// ========== PLUGIN SYSTEM ==========

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
};

std::map<std::string, FunctionEntry> g_functions;

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


void register_builtin_functions() {
    g_functions["sqrt"] = {"sqrt", 1, 1, [](const double* args, int nargs) {
        if (nargs != 1) throw std::runtime_error("sqrt expects 1 argument");
        if (args[0] < 0) throw std::runtime_error("sqrt domain error");
        return std::sqrt(args[0]);
    }};
    
    g_functions["max"] = {"max", 2, 2, [](const double* args, int nargs) {
        if (nargs != 2) throw std::runtime_error("max expects 2 arguments");
        return max(args[0], args[1]);
    }};

     g_functions["min"] = {"min", 2, 2, [](const double* args, int nargs) {
        if (nargs != 2) throw std::runtime_error("min expects 2 arguments");
        return min(args[0], args[1]);
    }};
}

std::vector<HMODULE> g_loaded_modules;

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

int main() {
    std::cout << "Calculator - Type 'quit' to exit\n";
    

    register_builtin_functions();
    
    load_plugins_from_folder("./plugins");
    
    std::cout << "Available functions: ";
    for (const auto& [name, _] : g_functions) {
        std::cout << name << " ";
    }
    std::cout << "\n";

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