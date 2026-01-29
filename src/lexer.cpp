#include <map>

#include "lexer.h"

std::map<std::string, TokenKind> keywordMap = {
    {"local", TokenKind::Local},
    {"function", TokenKind::Function},
    {"end", TokenKind::End},
    {"return", TokenKind::Return},
    {"if", TokenKind::If},
    {"then", TokenKind::Then},
    {"else", TokenKind::Else},
    {"elseif", TokenKind::ElseIf},

    {"true", TokenKind::True},
    {"false", TokenKind::False},
    {"nil", TokenKind::Nil},
    // logical ops
    {"not", TokenKind::Not},
    {"and", TokenKind::And},
    {"or", TokenKind::Or},
};

Token Lexer::getNextToken() {
    char c = peek();
    std::string lexeme;

    if (c == '\0') {
        return tok(TokenKind::Eof, "");
    } else if (isspace(c)) {
        advance();
        return getNextToken();
    } else if (isalpha(c) || c == '_') {
        while (isalnum(peek()) || peek() == '_') {
            lexeme += advance();
        }

        auto keywordIt = keywordMap.find(lexeme);
        if (keywordIt != keywordMap.end()) {
            return tok(keywordIt->second, std::move(lexeme));
        }

        return tok(TokenKind::Identifier, std::move(lexeme));
    } else if (isdigit(c)) {
        while (isdigit(peek())) {
            lexeme += advance();
        }
        return tok(TokenKind::Number, std::move(lexeme));
    } else if (c == '"') {
        advance(); // consume opening quote
        while (peek() != '"' && peek() != '\0') {
            lexeme += advance();
        }
        advance(); // consume closing quote
        return tok(TokenKind::String, std::move(lexeme));
    } else {
        // Handle single-character tokens
        switch (c) {
        case '(':
            advance();
            return tok(TokenKind::LParen, "(");
        case ')':
            advance();
            return tok(TokenKind::RParen, ")");
        case '{':
            advance();
            return tok(TokenKind::LBrace, "{");
        case '}':
            advance();
            return tok(TokenKind::RBrace, "}");
        case ':':
            advance();
            return tok(TokenKind::MethodAccess, ":");
        case ',':
            advance();
            return tok(TokenKind::Comma, ",");
        case '+':
            advance();
            return tok(TokenKind::Plus, "+");
        case '-':
            advance();
            return tok(TokenKind::Minus, "-");
        case '*':
            advance();
            return tok(TokenKind::Star, "*");
        case '/':
            advance();
            return tok(TokenKind::Slash, "/");
        case '.':
            advance();
            return tok(TokenKind::MemberAccess, ".");
        case '<': {
            advance();
            if (peek() == '=') {
                advance();
                return tok(TokenKind::LessEqual, "<=");
            }
            return tok(TokenKind::Less, "<");
        }
        case '>': {
            advance();
            if (peek() == '=') {
                advance();
                return tok(TokenKind::GreaterEqual, ">=");
            }
            return tok(TokenKind::Greater, ">");
        }
        case '=': {
            advance();
            if (peek() == '=') {
                advance();
                return tok(TokenKind::Equal, "==");
            }
            return tok(TokenKind::Assign, "=");
        }
        }
    }

    return Token{TokenKind::Eof, "", line, column};
}
