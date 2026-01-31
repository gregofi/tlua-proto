#pragma once

#include "ast.h"
#include "visitor.h"
#include <string>
#include <vector>

class TypedAstPrinter : public Visitor {
  public:
    std::string print(Program& program);
    std::string print(Expr& expr);
    std::string print(Stmt& stmt);

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

    void parenthesize(const std::string& name, const std::vector<Expr*>& exprs);
    void parenthesize(const std::string& name, const std::vector<std::unique_ptr<Expr>>& exprs);
};
