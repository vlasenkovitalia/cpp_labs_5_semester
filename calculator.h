#pragma once
#include <vector>
#include <map>
#include "token.h"

struct FunctionEntry;  // Forward declaration

struct OpInfo {
    int precedence;
    bool right_assoc;
};

extern std::map<std::string, OpInfo> op_info;

std::vector<Token> tokenize(const std::string& s);
std::vector<RPNItem> shunting_yard(const std::vector<Token>& tokens);
double evaluate_rpn(const std::vector<RPNItem>& rpn);