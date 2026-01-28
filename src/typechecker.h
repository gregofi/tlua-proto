#pragma once
#include "ast.h"
#include "environment.h"
#include "visitor.h"

class TypeChecker : public Visitor {
  public:
    TypeChecker();

    // Expression visitors
    void visit(StringExpr& expr) override;
    void visit(NumberExpr& expr) override;
    void visit(NilExpr& expr) override;
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

    Environment& getEnv() { return env; }

  private:
    Environment env;
};
