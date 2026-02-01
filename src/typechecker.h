#pragma once
#include "ast.h"
#include "environment.h"
#include "visitor.h"

#include <stdexcept>

class TypeCheckError : public std::runtime_error {
  public:
    explicit TypeCheckError(const std::string& message) : std::runtime_error(message) {}
};

class TypeChecker : public Visitor {
  public:
    TypeChecker();

    void typeCheck(Program& program) {
        for (auto& stmt : program.statements) {
            stmt->accept(*this);
        }
    }

    // Expression visitors
    void visit(StringExpr& expr) override;
    void visit(NumberExpr& expr) override;
    void visit(NilExpr& expr) override;
    void visit(BooleanExpr& expr) override;
    void visit(TableExpr& expr) override;
    void visit(VarExpr& expr) override;
    void visit(UnaryOpExpr& expr) override;
    void visit(BinOpExpr& expr) override;
    void visit(IndexExpr& expr) override;
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
    TypeCheckError error(const std::string& message) const { return TypeCheckError(message); }
    Type* resolveTypeAnnotation(const TypeAnnotation& annotation);
    Environment env;
};
