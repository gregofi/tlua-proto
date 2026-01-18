#pragma once
#include <string>
#include <vector>

enum class TokenKind {
    // literals
    Identifier,
    Number,
    String,

    // keywords
    Local, Function, End, Return, If, Then, Else, ElseIf,

    // symbols
    LParen, RParen, LBrace, RBrace,
    Colon, Comma, Assign,

    // operators
    Plus, Minus, Star, Slash,
    Equal, EqualEqual, NotEqual, Less, Greater,
    LessEqual, GreaterEqual,
    And, Or, Not,
    Concat,
    MemberAccess, MethodAccess,

    Eof,
};

inline const char* tokenKindToStr(TokenKind kind) {
    static const char* tokenKindToString[] = {
        "Identifier",
        "Number",
        "String",

        "Local", "Function", "End", "Return", "If", "Then", "Else", "ElseIf",

        "LParen", "RParen", "LBrace", "RBrace",
        "Colon", "Comma", "Assign",

        "Plus", "Minus", "Star",  "Slash",
        "Equal", "EqualEqual", "NotEqual", "Less", "Greater",
        "LessEqual", "GreaterEqual",
        "And", "Or", "Not",
        "Concat",
        "MemberAccess", "MethodAccess",

        "Eof",
    };
    return tokenKindToString[static_cast<int>(kind)];
}

enum class BinOpType {
    Add,
    Sub,
    Mul,
    Div,
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    And,
    Or,
    Concat,
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    int line;
    int column;
};

class Lexer {
public:
    Lexer(const std::string& source)
        : source(source), position(0), line(1), column(1) {}
    Token getNextToken();

    static std::vector<Token> tokenize(const std::string& source) {
        Lexer lexer(source);
        std::vector<Token> tokens;
        Token token;
        do {
            token = lexer.getNextToken();
            tokens.push_back(token);
        } while (token.kind != TokenKind::Eof);
        return tokens;
    }
private:
    char peek() const {
        if (position >= source.size()) return '\0';
        return source[position];
    }

    char advance() {
        if (position >= source.size()) return '\0';
        char current = source[position++];
        if (current == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        return current;
    }

    Token tok(TokenKind kind, std::string lexeme) {
        return Token{kind, lexeme, line, column - static_cast<int>(lexeme.size())};
    }

    std::string source;
    size_t position;
    int line;
    int column;
};
