#pragma once

#include "ast.h"
#include "visitor.h"
#include <string>

/// Visitor that converts a typed Lua AST back to normal Lua source code.
/// Type annotations are stripped, producing valid Lua output.
class LuaCodegen : public Visitor {
  public:
    std::string generate(Program& program);
    std::string generate(Expr& expr);
    std::string generate(Stmt& stmt);

    // Expression visitors
    void visit(StringExpr& expr) override;
    void visit(NumberExpr& expr) override;
    void visit(NilExpr& expr) override;
    void visit(BooleanExpr& expr) override;
    void visit(TableExpr& expr) override;
    void visit(VarExpr& expr) override;
    void visit(UnaryOpExpr& expr) override;
    void visit(BinOpExpr& expr) override;
    void visit(MemberAccessExpr& expr) override;
    void visit(MethodAccessExpr& expr) override;
    void visit(FunCallExpr& expr) override;

    // Statement visitors
    void visit(FunDecl& stmt) override;
    void visit(VarDecl& stmt) override;
    void visit(VarDecls& stmt) override;
    void visit(IfStmt& stmt) override;
    void visit(ReturnStmt& stmt) override;
    void visit(BlockStmt& stmt) override;
    void visit(FunCallStmt& stmt) override;
    void visit(AssignStmt& stmt) override;

  private:
    std::string result;
    int indent_level = 0;

    std::string indent() const;
    void newline();
    std::string generateExprString(Expr& expr);
};
