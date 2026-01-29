#pragma once
#include <format>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

#include "lexer.h"
#include "type.h"
#include "visitor.h"

struct Ast {
    virtual ~Ast() = default;
    virtual std::string toSExpr() const = 0;
    virtual void accept(Visitor& visitor) = 0;
};

struct Stmt : Ast {};

struct Program {
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct Expr : Ast {
    Type* type = nullptr;
};

struct StringExpr : Expr {
    StringExpr(std::string v) : val(std::move(v)) {}
    std::string val;

    std::string toSExpr() const override { return std::format("(string \"{}\")", val); }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct NumberExpr : Expr {
    NumberExpr(double v) : val(v) {}
    double val;

    std::string toSExpr() const override { return std::format("(number {})", val); }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct NilExpr : Expr {
    std::string toSExpr() const override { return "(nil)"; }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct BooleanExpr : Expr {
    BooleanExpr(bool v) : val(v) {}
    bool val;

    std::string toSExpr() const override {
        return std::format("(boolean {})", val ? "true" : "false");
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct VarExpr : Expr {
    VarExpr(std::string n) : name(std::move(n)) {}
    std::string name;

    std::string toSExpr() const override { return std::format("(var {})", name); }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct UnaryOpExpr : Expr {
    UnaryOpExpr(TokenKind o, std::unique_ptr<Expr> r) : op(o), right(std::move(r)) {}
    TokenKind op;
    std::unique_ptr<Expr> right;

    std::string toSExpr() const override {
        return std::format("({} {})", tokenKindToStr(op), right->toSExpr());
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct BinOpExpr : Expr {
    BinOpExpr(std::unique_ptr<Expr> l, TokenKind o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    std::unique_ptr<Expr> left;
    TokenKind op;
    std::unique_ptr<Expr> right;

    std::string toSExpr() const override {
        return std::format("({} {} {})", tokenKindToStr(op), left->toSExpr(), right->toSExpr());
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct MemberAccessExpr : Expr {
    MemberAccessExpr(std::unique_ptr<Expr> obj, std::string member)
        : object(std::move(obj)), member_name(std::move(member)) {}
    std::unique_ptr<Expr> object;
    std::string member_name;

    std::string toSExpr() const override {
        return std::format("(. {} {})", object->toSExpr(), member_name);
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct MethodAccessExpr : Expr {
    MethodAccessExpr(std::unique_ptr<Expr> obj, std::string method)
        : object(std::move(obj)), method_name(std::move(method)) {}
    std::unique_ptr<Expr> object;
    std::string method_name;

    std::string toSExpr() const override {
        return std::format("(: {} {})", object->toSExpr(), method_name);
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct FunCallExpr : Expr {
    FunCallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> arguments)
        : callee(std::move(callee)), args(std::move(arguments)) {}
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;

    std::string toSExpr() const override {
        auto argsStr = std::accumulate(
            args.begin(), args.end(), std::string{},
            [](std::string acc, auto&& expr) { return acc + " " + expr->toSExpr(); });
        // {}{} to avoid initial space
        return std::format("(call {}{})", callee->toSExpr(), argsStr);
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct Decl : Stmt {
    Decl(std::string name, bool local) : name(std::move(name)), local(local) {}
    std::string name;
    bool local;
};

struct FunDecl : Decl {
    FunDecl(std::string name, bool local, std::optional<std::string> this_name, bool method,
            std::vector<std::string> params, std::unique_ptr<Stmt> body)
        : Decl{std::move(name), local}, this_name(std::move(this_name)), method(method),
          params(std::move(params)), body(std::move(body)) {}
    // Fundecls may be methods:
    // function obj:method(params)
    // Also, they may be regular functions with dot notation:
    // function obj.method(params)
    // In both cases, `this_name` is set to "obj".
    // If it's a method, `is_method` is true.
    std::optional<std::string> this_name;
    bool method;

    std::vector<std::string> params;
    std::unique_ptr<Stmt> body;

    std::string toSExpr() const override {
        auto paramsStr = std::accumulate(params.begin(), params.end(), std::string{},
                                         [](std::string acc, const std::string& param) {
                                             return acc + (acc.empty() ? "" : " ") + param;
                                         });
        return std::format("(fun {} {} ({}) {})", local ? "local" : "global", name, paramsStr,
                           body->toSExpr());
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

/// Variable declaration
struct VarDecl : Decl {
    VarDecl(std::string name, std::unique_ptr<Expr> init)
        : Decl{std::move(name), true}, // VarDecls are always local
          init_expr(std::move(init)) {}
    std::unique_ptr<Expr> init_expr;

    std::string toSExpr() const override {
        return std::format("(var-decl {} {})", name, init_expr->toSExpr());
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct VarDecls : Stmt {
    std::vector<std::unique_ptr<VarDecl>> decls;

    std::string toSExpr() const override {
        std::string result = "(var-decls";
        for (const auto& decl : decls) {
            result += " " + decl->toSExpr();
        }
        result += ")";
        return result;
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct IfStmt : Stmt {
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then_b,
           std::unique_ptr<Stmt> else_b = nullptr)
        : condition(std::move(cond)), then_branch(std::move(then_b)),
          else_branch(std::move(else_b)) {}

    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch;

    std::string toSExpr() const override {
        std::string result =
            std::format("(if {} (then {}))", condition->toSExpr(), then_branch->toSExpr());
        if (else_branch) {
            result = std::format("(if {} (then {}) (else {}))", condition->toSExpr(),
                                 then_branch->toSExpr(), else_branch->toSExpr());
        }
        return result;
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct ReturnStmt : Stmt {
    ReturnStmt(std::vector<std::unique_ptr<Expr>> vals) : return_values(std::move(vals)) {}
    std::vector<std::unique_ptr<Expr>> return_values;

    std::string toSExpr() const override {
        std::string result = "(return";
        for (const auto& val : return_values) {
            result += " " + val->toSExpr();
        }
        result += ")";
        return result;
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    std::string toSExpr() const override {
        auto stmts = std::accumulate(
            statements.begin(), statements.end(), std::string{},
            [](std::string acc, auto&& stmt) { return acc + " " + stmt->toSExpr(); });
        return std::format("(block{})", stmts);
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct FunCallStmt : Stmt {
    explicit FunCallStmt(std::unique_ptr<FunCallExpr> c) : call(std::move(c)) {}
    std::unique_ptr<FunCallExpr> call;

    std::string toSExpr() const override { return call->toSExpr(); }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct AssignStmt : Stmt {
    AssignStmt(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : left(std::move(l)), right(std::move(r)) {}
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    std::string toSExpr() const override {
        return std::format("(assign {} {})", left->toSExpr(), right->toSExpr());
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};
