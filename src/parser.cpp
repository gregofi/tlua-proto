// Recursive descent parser
// Pratt parsing for expressions (source:
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html)

#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include <cassert>
#include <format>
#include <iostream>
#include <vector>

Program Parser::parse() { return parseTopLevel(); }

bool Parser::match(TokenKind kind) {
    if (position < tokens.size() && tokens[position].kind == kind) {
        position++;
        return true;
    }
    return false;
}

Token Parser::peek() const {
    if (position < tokens.size()) {
        return tokens[position];
    }
    return Token{TokenKind::Eof, "", -1, -1};
}

Program Parser::parseTopLevel() {
    Program program;
    while (position < tokens.size() && tokens[position].kind != TokenKind::Eof) {
        program.statements.emplace_back(parseStmt());
    }
    return program;
}

std::pair<int, int> Parser::opPrecedence(TokenKind kind) const {
    switch (kind) {
    case TokenKind::And:
    case TokenKind::Or:
        return {10, 11};
    case TokenKind::Equal:
    case TokenKind::NotEqual:
    case TokenKind::Less:
    case TokenKind::Greater:
    case TokenKind::LessEqual:
    case TokenKind::GreaterEqual:
        return {20, 21};
    case TokenKind::Plus:
    case TokenKind::Minus:
        return {30, 31};
    case TokenKind::Star:
    case TokenKind::Slash:
        return {40, 41};
    case TokenKind::Concat:
        return {50, 51};
    case TokenKind::MemberAccess:
    case TokenKind::MethodAccess:
        return {60, 61};
    default:
        return {-1, -1}; // causes the parser to stop parsing further
    }
}

std::optional<int> Parser::prefixPrecedence(TokenKind kind) const {
    switch (kind) {
    case TokenKind::Minus:
    case TokenKind::Not:
        return 70;
    default:
        return std::nullopt;
    }
}

std::optional<int> Parser::postfixPrecedence(TokenKind kind) const {
    switch (kind) {
    case TokenKind::LParen:
        return 80; // function call
    default:
        return std::nullopt;
    }
}

std::unique_ptr<TableExpr> Parser::parseTableExpr() {
    std::vector<std::unique_ptr<Expr>> arrayElements;
    std::vector<TableKeyVal> keyValueElements;
    while (!match(TokenKind::RBrace)) {
        auto expr = parseExpr();
        if (match(TokenKind::Assign)) {
            auto id = dynamic_cast<VarExpr*>(expr.get());
            if (!id) {
                throw ParseError("Expected identifier in table key=value assignment");
            }
            auto valueExpr = parseExpr();
            TableKeyVal kv{id->name, std::move(valueExpr)};
            keyValueElements.emplace_back(std::move(kv));
        } else {
            arrayElements.emplace_back(std::move(expr));
        }

        if (!match(TokenKind::Comma)) {
            if (peek().kind != TokenKind::RBrace) {
                throw errorExpectedTok("',' or '}' in table constructor");
            }
        }
    }
    return std::make_unique<TableExpr>(std::move(arrayElements), std::move(keyValueElements));
}

std::unique_ptr<Expr> Parser::parseAtomExpr() {
    if (match(TokenKind::Number)) {
        return std::make_unique<NumberExpr>(std::stod(tokens[position - 1].lexeme));
    } else if (match(TokenKind::Identifier)) {
        return std::make_unique<VarExpr>(tokens[position - 1].lexeme);
    } else if (match(TokenKind::String)) {
        return std::make_unique<StringExpr>(tokens[position - 1].lexeme);
    } else if (match(TokenKind::Nil)) {
        return std::make_unique<NilExpr>();
    } else if (match(TokenKind::True)) {
        return std::make_unique<BooleanExpr>(true);
    } else if (match(TokenKind::False)) {
        return std::make_unique<BooleanExpr>(false);
    } else if (match(TokenKind::LBrace)) {
        return parseTableExpr();
    } else if (match(TokenKind::LParen)) {
        auto expr = parseExpr();
        if (!match(TokenKind::RParen)) {
            throw errorExpectedTok("')' after expression");
        }
        return expr;
    }

    throw ParseError(std::format("Expected atomic expression, but found '{}'",
                                 tokenKindToStr(tokens[position].kind)));
}

std::unique_ptr<Expr> binaryExpr(std::unique_ptr<Expr> lhs, TokenKind op,
                                 std::unique_ptr<Expr> rhs) {
    return std::make_unique<BinOpExpr>(std::move(lhs), op, std::move(rhs));
}

std::unique_ptr<Expr> unaryExpr(TokenKind op, std::unique_ptr<Expr> rhs) {
    return std::make_unique<UnaryOpExpr>(op, std::move(rhs));
}

std::unique_ptr<Expr> Parser::parsePostfixExpr(std::unique_ptr<Expr> lhs, TokenKind op) {
    // funcall
    if (match(TokenKind::LParen)) {
        std::vector<std::unique_ptr<Expr>> args;
        while (true) {
            if (peek().kind == TokenKind::RParen) {
                break;
            }

            args.emplace_back(parseExpr());
            if (!match(TokenKind::Comma)) {
                break;
            }
        };

        if (!match(TokenKind::RParen)) {
            throw errorExpectedTok("')' after function call arguments");
        }
        return std::make_unique<FunCallExpr>(std::move(lhs), std::move(args));
    } else {
        throw errorExpectedTok("postfix operator");
    }
}

// Pratt parser
std::unique_ptr<Expr> Parser::parseExpr(int prevPrec) {
    auto prefixTok = peek();
    auto prefixPrecOpt = prefixPrecedence(prefixTok.kind);

    std::unique_ptr<Expr> lhs;
    if (prefixPrecOpt) { // We have a prefix operator
        match(prefixTok.kind);
        auto rhs = parseExpr(*prefixPrecOpt);
        lhs = unaryExpr(prefixTok.kind, std::move(rhs));
    } else {
        lhs = parseAtomExpr();
    }

    while (true) {
        auto currentToken = peek();

        auto postfixPrecOpt = postfixPrecedence(currentToken.kind);
        if (postfixPrecOpt) {
            lhs = parsePostfixExpr(std::move(lhs), currentToken.kind);
            continue;
        }

        auto [lprec, rprec] = opPrecedence(currentToken.kind);
        if (lprec < prevPrec) {
            break;
        }

        match(currentToken.kind);
        auto rhs = parseExpr(rprec);
        lhs = binaryExpr(std::move(lhs), currentToken.kind, std::move(rhs));
    }

    return lhs;
}

std::unique_ptr<Stmt> Parser::parseStmt() {
    if (peek().kind == TokenKind::Local || peek().kind == TokenKind::Function) {
        return parseDecl();
    } else if (match(TokenKind::Return)) {
        return parseReturnStmt();
        // In global scope, a function call or a
        // variable (or table member) assignment can be a statement.
    } else if (peek().kind == TokenKind::Identifier) {
        auto expr = parseExpr();
        if (auto funCall = dynamic_cast<FunCallExpr*>(expr.get())) {
            auto _ = expr.release();
            return std::make_unique<FunCallStmt>(std::unique_ptr<FunCallExpr>(funCall));
        } else {
            if (match(TokenKind::Assign)) {
                auto valueExpr = parseExpr();
                ;
                return std::make_unique<AssignStmt>(std::move(expr), std::move(valueExpr));
            } else {
                throw errorExpectedTok("function call statement or assignment");
            }
        }
    }
    if (match(TokenKind::If)) {
        return parseIfStmt();
    }

    throw errorExpectedTok("declaration, return statement, or if statement");
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    std::vector<std::unique_ptr<Expr>> returnValues;
    auto expr = parseExpr();
    returnValues.emplace_back(std::move(expr));
    while (match(TokenKind::Comma)) {
        expr = parseExpr();
        returnValues.emplace_back(std::move(expr));
    }
    return std::make_unique<ReturnStmt>(std::move(returnValues));
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    auto condition = parseExpr();
    if (!match(TokenKind::Then)) {
        throw errorExpectedTok("'then' after if condition");
    }
    auto thenBlock = std::make_unique<BlockStmt>();
    while (!match(TokenKind::Else) && !match(TokenKind::ElseIf) && !match(TokenKind::End)) {
        thenBlock->statements.emplace_back(parseStmt());
    }

    std::unique_ptr<Stmt> elseStmt = nullptr;
    if (tokens[position - 1].kind == TokenKind::ElseIf) {
        elseStmt = parseIfStmt();
    } else if (tokens[position - 1].kind == TokenKind::Else) {
        auto elseBlock = std::make_unique<BlockStmt>();
        while (!match(TokenKind::End)) {
            elseBlock->statements.emplace_back(parseStmt());
        }
        elseStmt = std::move(elseBlock);
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBlock),
                                    std::move(elseStmt));
}

std::unique_ptr<VarDecl> Parser::parseVarDecl() {
    if (!match(TokenKind::Identifier)) {
        throw errorExpectedTok("variable name");
    }
    std::string varName = tokens[position - 1].lexeme;

    if (!match(TokenKind::Assign)) {
        return std::make_unique<VarDecl>(varName, std::make_unique<NilExpr>());
    }
    auto initExpr = parseExpr();
    return std::make_unique<VarDecl>(varName, std::move(initExpr));
}

std::unique_ptr<Decl> Parser::parseDecl() {
    if (match(TokenKind::Local)) {
        if (match(TokenKind::Function)) {
            return parseFunDecl(true);
        }
        return parseVarDecl();
    } else if (match(TokenKind::Function)) {
        return parseFunDecl(false);
    }
    throw errorExpectedTok("'local' or 'function' for declaration");
}

std::unique_ptr<FunDecl> Parser::parseFunDecl(bool local) {
    if (!match(TokenKind::Identifier)) {
        throw errorExpectedTok("function name");
    }
    std::string functionName = tokens[position - 1].lexeme;

    if (!match(TokenKind::LParen)) {
        throw errorExpectedTok("'(' after function name");
    }

    std::vector<std::string> parameters;
    if (!match(TokenKind::RParen)) {
        do {
            if (!match(TokenKind::Identifier)) {
                throw errorExpectedTok("parameter name");
            }
            parameters.push_back(tokens[position - 1].lexeme);
        } while (match(TokenKind::Comma));

        if (!match(TokenKind::RParen)) {
            throw errorExpectedTok("')' after parameters");
        }
    }

    auto body = std::make_unique<BlockStmt>();
    while (!match(TokenKind::End)) {
        body->statements.emplace_back(parseStmt());
    }

    return std::make_unique<FunDecl>(functionName, local,
                                     std::nullopt, // TODO
                                     false,        // TODO
                                     std::move(parameters), std::move(body));
}
