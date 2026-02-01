#pragma once
#include <format>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lexer.h"
#include "type.h"
#include "visitor.h"

// Helper for std::visit with overloaded lambdas
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Forward declaration
class TypeAnnotation;

// Basic type annotation (number, string, boolean, nil)
struct BasicTypeAnnotation {
    enum class Kind { Number, String, Boolean, Nil };
    Kind kind;
};

// Function type annotation: (param1, param2, ...) -> returnType
struct FunctionTypeAnnotation {
    std::vector<TypeAnnotation> paramTypes;
    std::unique_ptr<TypeAnnotation> returnType;
};

// Table type annotation: { field1: type1, field2: type2, ... }
struct TableTypeAnnotation {
    std::map<std::string, TypeAnnotation> fields;
};

// Array type annotation: type[]
struct ArrayTypeAnnotation {
    std::unique_ptr<TypeAnnotation> elementType;
};

// Union type annotation: type1 | type2 | ...
struct UnionTypeAnnotation {
    std::vector<TypeAnnotation> types;
};

class TypeAnnotation {
    using Variant = std::variant<BasicTypeAnnotation, FunctionTypeAnnotation,
                                  TableTypeAnnotation, ArrayTypeAnnotation, UnionTypeAnnotation>;
    Variant annotation;

public:
    template <typename T>
    TypeAnnotation(T&& value) : annotation(std::forward<T>(value)) {}

    const Variant& getVariant() const { return annotation; }

    std::string toString() const {
        return std::visit(
            overloaded{
                [](const BasicTypeAnnotation& arg) -> std::string {
                    switch (arg.kind) {
                    case BasicTypeAnnotation::Kind::Number:
                        return "number";
                    case BasicTypeAnnotation::Kind::String:
                        return "string";
                    case BasicTypeAnnotation::Kind::Boolean:
                        return "boolean";
                    case BasicTypeAnnotation::Kind::Nil:
                        return "nil";
                    }
                    return "unknown";
                },
                [](const FunctionTypeAnnotation& arg) -> std::string {
                    std::string result = "(";
                    for (size_t i = 0; i < arg.paramTypes.size(); ++i) {
                        if (i > 0)
                            result += ", ";
                        result += arg.paramTypes[i].toString();
                    }
                    result += ") -> ";
                    result += arg.returnType ? arg.returnType->toString() : "void";
                    return result;
                },
                [](const TableTypeAnnotation& arg) -> std::string {
                    std::string result = "{";
                    bool first = true;
                    for (const auto& [field, type] : arg.fields) {
                        if (!first)
                            result += ", ";
                        first = false;
                        result += field + ": " + type.toString();
                    }
                    result += "}";
                    return result;
                },
                [](const ArrayTypeAnnotation& arg) -> std::string {
                    return arg.elementType->toString() + "[]";
                },
                [](const UnionTypeAnnotation& arg) -> std::string {
                    std::string result;
                    for (size_t i = 0; i < arg.types.size(); ++i) {
                        if (i > 0)
                            result += " | ";
                        result += arg.types[i].toString();
                    }
                    return result;
                }},
            annotation);
    }
};

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

struct TableExpr : Expr {
    TableExpr(std::vector<std::unique_ptr<Expr>> arr,
              std::map<std::string, std::unique_ptr<Expr>> map)
        : arrayPart(std::move(arr)), mapPart(std::move(map)) {}
    std::vector<std::unique_ptr<Expr>> arrayPart;
    std::map<std::string, std::unique_ptr<Expr>> mapPart;

    std::string toSExpr() const override {
        auto arrayStr = std::accumulate(
            arrayPart.begin(), arrayPart.end(), std::string{},
            [](std::string acc, auto&& expr) { return acc + " " + expr->toSExpr(); });
        auto mapStr = std::accumulate(
            mapPart.begin(), mapPart.end(), std::string{}, [](std::string acc, auto&& kv) {
                return acc + " (" + kv.first + " " + kv.second->toSExpr() + ")";
            });
        return std::format("(table (array{} ) (map{} ))", arrayStr, mapStr);
    }

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

struct IndexExpr : Expr {
    IndexExpr(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx)
        : object(std::move(obj)), index(std::move(idx)) {}
    std::unique_ptr<Expr> object;
    std::unique_ptr<Expr> index;

    std::string toSExpr() const override {
        return std::format("([] {} {})", object->toSExpr(), index->toSExpr());
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
    Type* type = nullptr;
};

struct Parameter {
    std::string name;
    std::optional<TypeAnnotation> typeAnnotation;

    Parameter(std::string n, std::optional<TypeAnnotation> ta = std::nullopt)
        : name(std::move(n)), typeAnnotation(std::move(ta)) {}

    std::string toString() const {
        if (typeAnnotation.has_value()) {
            return std::format("{}:{}", name, typeAnnotation->toString());
        } else {
            return name;
        }
    }
};

struct FunDecl : Decl {
    FunDecl(std::string name, bool local, std::optional<std::string> thisName, bool method,
            std::vector<Parameter> params, std::unique_ptr<Stmt> body,
            std::optional<TypeAnnotation> retType = std::nullopt)
        : Decl{std::move(name), local}, thisName(std::move(thisName)), method(method),
          params(std::move(params)), body(std::move(body)), returnTypeAnnotation(std::move(retType)) {}
    // Fundecls may be methods:
    // function obj:method(params)
    // Also, they may be regular functions with dot notation:
    // function obj.method(params)
    // In both cases, `thisName` is set to "obj".
    // If it's a method, `method` is true.
    std::optional<std::string> thisName;
    bool method;

    std::vector<Parameter> params;
    std::unique_ptr<Stmt> body;
    std::optional<TypeAnnotation> returnTypeAnnotation;

    std::string toSExpr() const override {
        auto paramsStr = std::accumulate(params.begin(), params.end(), std::string{},
                                         [](std::string acc, const Parameter& param) {
                                             std::string paramStr = param.name;
                                             if (param.typeAnnotation.has_value()) {
                                                 paramStr += ":" + param.typeAnnotation->toString();
                                             }
                                             return acc + (acc.empty() ? "" : " ") + paramStr;
                                         });
        std::string retTypeStr = returnTypeAnnotation.has_value() 
                                    ? " -> " + returnTypeAnnotation->toString()
                                    : "";
        return std::format("(fun {} {}{} ({}) {})", local ? "local" : "global", name, 
                           retTypeStr, paramsStr, body->toSExpr());
    }
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

/// Variable declaration
struct VarDecl : Decl {
    VarDecl(std::string name, std::unique_ptr<Expr> init,
            std::optional<TypeAnnotation> typeAnnotation = std::nullopt)
        : Decl{std::move(name), true}, // VarDecls are always local
          initExpr(std::move(init)), typeAnnotation(std::move(typeAnnotation)) {}
    std::unique_ptr<Expr> initExpr;
    std::optional<TypeAnnotation> typeAnnotation;

    std::string toSExpr() const override {
        std::string nameWithType = name;
        if (typeAnnotation.has_value()) {
            nameWithType += ":" + typeAnnotation->toString();
        }
        return std::format("(var-decl {} {})", nameWithType, initExpr->toSExpr());
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
