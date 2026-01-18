#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lexer.h"
#include "type.h"

struct Ast {virtual ~Ast() = default; };

struct Stmt: Ast {};

struct Expr: Ast {
    Type* type = nullptr;
};

struct StringExpr: Expr {
    StringExpr(std::string v): val(std::move(v)) {}
    std::string val;
};

struct NumberExpr: Expr {
    NumberExpr(double v): val(v) {}
    double val;
};

struct VarExpr: Expr {
    VarExpr(std::string n): name(std::move(n)) {}
    std::string name;
};

struct UnaryOpExpr: Expr {
    UnaryOpExpr(
        TokenKind o,
        std::unique_ptr<Expr> r
    ) : op(o), right(std::move(r)) {}
    TokenKind op;
    std::unique_ptr<Expr> right;
};

struct BinOpExpr: Expr {
    BinOpExpr(
        std::unique_ptr<Expr> l,
        TokenKind o,
        std::unique_ptr<Expr> r
    ) : left(std::move(l)), op(o), right(std::move(r)) {}
    std::unique_ptr<Expr> left;
    TokenKind op;
    std::unique_ptr<Expr> right;
};

struct MemberAccessExpr: Expr {
    MemberAccessExpr(
        std::unique_ptr<Expr> obj,
        std::string member
    ) : object(std::move(obj)), member_name(std::move(member)) {}
    std::unique_ptr<Expr> object;
    std::string member_name;
};

struct MethodAccessExpr: Expr {
    MethodAccessExpr(
        std::unique_ptr<Expr> obj,
        std::string method
    ) : object(std::move(obj)), method_name(std::move(method)) {}
    std::unique_ptr<Expr> object;
    std::string method_name;
};

struct FunCallExpr: Expr {
    FunCallExpr(
        std::unique_ptr<Expr> callee,
        std::vector<std::unique_ptr<Expr>> arguments
    ) : callee(std::move(callee)), args(std::move(arguments)) {}
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
};

struct Decl: Stmt {
    Decl(std::string name, bool local)
        : name(std::move(name)), local(local) {}
    std::string name;
    bool local;
};

struct FunDecl: Decl {
    FunDecl(
        std::string name,
        bool local,
        std::optional<std::string> this_name,
        bool method,
        std::vector<std::string> params,
        std::vector<std::unique_ptr<Stmt>> body
    ): Decl{ std::move(name), local },
       this_name(std::move(this_name)),
       method(method),
       params(std::move(params)),
       body(std::move(body)) {}
    // Fundecls may be methods:
    // function obj:method(params)
    // Also, they may be regular functions with dot notation:
    // function obj.method(params)
    // In both cases, `this_name` is set to "obj".
    // If it's a method, `is_method` is true.
    std::optional<std::string> this_name;
    bool method;

    std::vector<std::string> params;
    std::vector<std::unique_ptr<Stmt>> body;
};

/// Variable declaration
struct VarDecl: Decl {
    VarDecl(std::string name, std::unique_ptr<Expr> init)
        : Decl{ std::move(name), true }, // VarDecls are always local
          init_expr(std::move(init)) {}
    std::string name;
    std::unique_ptr<Expr> init_expr;
};

struct VarDecls: Stmt {
    std::vector<std::unique_ptr<VarDecl>> decls;
};

struct IfStmt: Stmt {
    IfStmt(
        std::unique_ptr<Expr> cond,
        std::vector<std::unique_ptr<Stmt>> then_b,
        std::vector<std::unique_ptr<Stmt>> else_b
    )
        : condition(std::move(cond)),
          then_body(std::move(then_b)),
          else_body(std::move(else_b)) {}

    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> then_body;
    std::vector<std::unique_ptr<Stmt>> else_body;
};

struct ReturnStmt: Stmt {
    ReturnStmt(std::vector<std::unique_ptr<Expr>> vals)
        : return_values(std::move(vals)) {}
    std::vector<std::unique_ptr<Expr>> return_values;
};

struct BlockStmt: Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct FunCallStmt: Stmt {
    std::unique_ptr<FunCallExpr> call;
};

