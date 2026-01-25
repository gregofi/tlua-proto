#pragma once
#include "ast.h"
#include "lexer.h"
#include <format>
#include <memory>
#include <vector>

struct Program {
    std::vector<std::unique_ptr<Stmt>> statements;
};

class ParseError : public std::runtime_error {
  public:
    ParseError(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
  public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens), position(0) {}
    Program parse();

  private:
    /// If the current token matches the given kind, consumes it and returns true.
    bool match(TokenKind kind);
    /// Consumes current and returns the next token, advancing the position.
    Token shift();
    Token peek() const;
    /// Restore the position to the last stashed point.
    void restore(size_t stashed) { position = stashed; }
    ParseError errorExpectedTok(const std::string& expected) const {
        return ParseError(std::format("Expected {}, but found '{}' ({}) at line {}, column {}",
                                      expected, tokens[position].lexeme,
                                      tokenKindToStr(tokens[position].kind), tokens[position].line,
                                      tokens[position].column));
    }

    std::pair<int, int> opPrecedence(TokenKind kind) const;
    /// Returns the precedence of a prefix operator.
    /// if the token is not a prefix operator, returns std::nullopt.
    std::optional<int> prefixPrecedence(TokenKind kind) const;
    /// Returns the precedence of a postfix operator.
    /// if the token is not a postfix operator, returns std::nullopt.
    std::optional<int> postfixPrecedence(TokenKind kind) const;

    Program parseTopLevel();

    std::unique_ptr<Expr> parseAtomExpr();
    std::unique_ptr<Expr> parseExpr(int prevPrec = 0);
    std::unique_ptr<Expr> parsePostfixExpr(std::unique_ptr<Expr> left, TokenKind op);
    std::optional<std::unique_ptr<Expr>> parseMemberAccess(std::unique_ptr<Expr> object);

    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<ReturnStmt> parseReturnStmt();
    std::unique_ptr<IfStmt> parseIfStmt();

    std::unique_ptr<Decl> parseDecl();
    std::unique_ptr<FunDecl> parseFunDecl(bool local);
    std::unique_ptr<VarDecl> parseVarDecl();
    std::vector<Token> tokens;
    size_t position;
};
