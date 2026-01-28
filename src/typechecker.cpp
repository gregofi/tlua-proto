#include "typechecker.h"
#include <stdexcept>

TypeChecker::TypeChecker() {
    env.pushScope(); // global scope
}

// Expression implementations

void TypeChecker::visit(StringExpr& expr) { expr.type = TypeFactory::instance().stringType(); }

void TypeChecker::visit(NumberExpr& expr) { expr.type = TypeFactory::instance().numberType(); }

void TypeChecker::visit(NilExpr& expr) { expr.type = TypeFactory::instance().nilType(); }

void TypeChecker::visit(VarExpr& expr) {
    Type* t = env.lookup(expr.name);
    if (t == nullptr) {
        throw std::runtime_error("Undefined variable: " + expr.name);
    }
    expr.type = t;
}

void TypeChecker::visit(UnaryOpExpr& expr) {
    expr.right->accept(*this);
    throw std::runtime_error("typeCheck not implemented for UnaryOpExpr");
}

void TypeChecker::visit(BinOpExpr& expr) {
    expr.left->accept(*this);
    expr.right->accept(*this);
    throw std::runtime_error("typeCheck not implemented for BinOpExpr");
}

void TypeChecker::visit(MemberAccessExpr& expr) {
    expr.object->accept(*this);
    throw std::runtime_error("typeCheck not implemented for MemberAccessExpr");
}

void TypeChecker::visit(MethodAccessExpr& expr) {
    expr.object->accept(*this);
    throw std::runtime_error("typeCheck not implemented for MethodAccessExpr");
}

void TypeChecker::visit(FunCallExpr& expr) {
    expr.callee->accept(*this);
    for (auto& arg : expr.args) {
        arg->accept(*this);
    }
    throw std::runtime_error("typeCheck not implemented for FunCallExpr");
}

// Statement implementations

void TypeChecker::visit(FunDecl& stmt) {
    throw std::runtime_error("typeCheck not implemented for FunDecl");
}

void TypeChecker::visit(VarDecl& stmt) {
    stmt.init_expr->accept(*this);
    env.define(stmt.name, stmt.init_expr->type);
}

void TypeChecker::visit(VarDecls& stmt) {
    for (auto& decl : stmt.decls) {
        decl->accept(*this);
    }
}

void TypeChecker::visit(IfStmt& stmt) {
    stmt.condition->accept(*this);
    throw std::runtime_error("typeCheck not implemented for IfStmt");
}

void TypeChecker::visit(ReturnStmt& stmt) {
    for (auto& val : stmt.return_values) {
        val->accept(*this);
    }
    throw std::runtime_error("typeCheck not implemented for ReturnStmt");
}

void TypeChecker::visit(BlockStmt& stmt) {
    env.pushScope();
    for (auto& s : stmt.statements) {
        s->accept(*this);
    }
    env.popScope();
}

void TypeChecker::visit(FunCallStmt& stmt) { stmt.call->accept(*this); }

void TypeChecker::visit(AssignStmt& stmt) {
    stmt.left->accept(*this);
    stmt.right->accept(*this);
    throw std::runtime_error("typeCheck not implemented for AssignStmt");
}
