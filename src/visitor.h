#pragma once

// Forward declarations for all AST node types
struct StringExpr;
struct NumberExpr;
struct NilExpr;
struct BooleanExpr;
struct TableExpr;
struct VarExpr;
struct UnaryOpExpr;
struct BinOpExpr;
struct MemberAccessExpr;
struct MethodAccessExpr;
struct FunCallExpr;
struct FunDecl;
struct VarDecl;
struct VarDecls;
struct IfStmt;
struct ReturnStmt;
struct BlockStmt;
struct FunCallStmt;
struct AssignStmt;

class Visitor {
  public:
    virtual ~Visitor() = default;

    // Expression visitors
    virtual void visit(StringExpr& expr) = 0;
    virtual void visit(NumberExpr& expr) = 0;
    virtual void visit(NilExpr& expr) = 0;
    virtual void visit(BooleanExpr& expr) = 0;
    virtual void visit(TableExpr& expr) = 0;
    virtual void visit(VarExpr& expr) = 0;
    virtual void visit(UnaryOpExpr& expr) = 0;
    virtual void visit(BinOpExpr& expr) = 0;
    virtual void visit(MemberAccessExpr& expr) = 0;
    virtual void visit(MethodAccessExpr& expr) = 0;
    virtual void visit(FunCallExpr& expr) = 0;

    // Statement visitors
    virtual void visit(FunDecl& stmt) = 0;
    virtual void visit(VarDecl& stmt) = 0;
    virtual void visit(VarDecls& stmt) = 0;
    virtual void visit(IfStmt& stmt) = 0;
    virtual void visit(ReturnStmt& stmt) = 0;
    virtual void visit(BlockStmt& stmt) = 0;
    virtual void visit(FunCallStmt& stmt) = 0;
    virtual void visit(AssignStmt& stmt) = 0;
};
