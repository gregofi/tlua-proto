#include <format>
#include <vector>
#include "parser.h"
#include "lexer.h"
#include "ast.h"

Program Parser::parse() {
    return parseTopLevel();
}

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

std::unique_ptr<Expr> Parser::parseAtomExpr() {
    if (match(TokenKind::Number)) {
        return std::make_unique<NumberExpr>(std::stod(tokens[position - 1].lexeme));
    } else if (match(TokenKind::Identifier)) {
        return std::make_unique<VarExpr>(tokens[position - 1].lexeme);
    } else if (match(TokenKind::String)) {
        return std::make_unique<StringExpr>(tokens[position - 1].lexeme);
    }

    throw ParseError(std::format("Expected atomic expression, but found '{}'", tokenKindToStr(tokens[position].kind)));
}

std::unique_ptr<Expr> binaryExpr(std::unique_ptr<Expr> lhs, TokenKind op, std::unique_ptr<Expr> rhs) {
    if (op == TokenKind::MemberAccess) {

    } else if (op == TokenKind::MethodAccess) {

    }
    return std::make_unique<BinOpExpr>(std::move(lhs), op, std::move(rhs));
}

// Pratt parser
std::unique_ptr<Expr> Parser::parseExpr(int prevPrec) {
    auto lhs = parseAtomExpr();

    while (true) {
        auto currentToken = peek();
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
    } if (match(TokenKind::If)) {
        return parseIfStmt();
    }

    throw ParseError(std::format("Expected statement, but found '{}'", tokenKindToStr(tokens[position].kind)));
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
    std::vector<std::unique_ptr<Stmt>> thenBody;
    while (!match(TokenKind::Else) && !match(TokenKind::End)) {
        thenBody.emplace_back(parseStmt());
    }
    std::vector<std::unique_ptr<Stmt>> elseBody;
    if (tokens[position - 1].kind == TokenKind::Else) {
        while (!match(TokenKind::End)) {
            elseBody.emplace_back(parseStmt());
        }
    }
    return std::make_unique<IfStmt>(
        std::move(condition),
        std::move(thenBody),
        std::move(elseBody)
    );
}

std::unique_ptr<VarDecl> Parser::parseVarDecl() {
    if (!match(TokenKind::Identifier)) {
        throw errorExpectedTok("variable name");
    }
    std::string varName = tokens[position - 1].lexeme;

    if (!match(TokenKind::Assign)) {
        throw errorExpectedTok("'=' after variable name");
    }
    auto initExpr = parseExpr();
    return std::make_unique<VarDecl>(
        varName,
        std::move(initExpr)
    );
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

    std::vector<std::unique_ptr<Stmt>> body;
    while (!match(TokenKind::End)) {
        body.emplace_back(parseStmt());
    }

    return std::make_unique<FunDecl>(
        functionName,
        local,
        std::nullopt, // TODO
        false, // TODO
        std::move(parameters),
        std::move(body)
    );
}
