#pragma once
#include <string>

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

struct RPNItem {
    Token token;
    int arg_count;
};
