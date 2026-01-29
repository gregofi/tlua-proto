#include "typechecker.h"

namespace {
bool isAny(Type* type) { return type->getKind() == TypeKind::Any; }

bool isNumber(Type* type) { return isAny(type) || type == TypeFactory::numberType(); }

bool isString(Type* type) { return isAny(type) || type == TypeFactory::stringType(); }
} // namespace

TypeChecker::TypeChecker() {
    env.pushScope(); // global scope
}

// Expression implementations

void TypeChecker::visit(StringExpr& expr) { expr.type = TypeFactory::stringType(); }

void TypeChecker::visit(NumberExpr& expr) { expr.type = TypeFactory::numberType(); }

void TypeChecker::visit(NilExpr& expr) { expr.type = TypeFactory::nilType(); }

void TypeChecker::visit(VarExpr& expr) {
    Type* t = env.lookup(expr.name);
    if (t == nullptr) {
        expr.type = TypeFactory::anyType();
    } else {
        expr.type = t;
    }
}

void TypeChecker::visit(UnaryOpExpr& expr) {
    expr.right->accept(*this);
    // TODO: typecheck operators
    expr.type = expr.right->type;
}

void TypeChecker::visit(BinOpExpr& expr) {
    expr.left->accept(*this);
    expr.right->accept(*this);
    auto* leftType = expr.left->type;
    auto* rightType = expr.right->type;
    switch (expr.op) {
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Star:
    case TokenKind::Slash: {
        if (!isNumber(leftType) || !isNumber(rightType)) {
            throw error("Type error: arithmetic operations require number types");
        }
        expr.type = TypeFactory::numberType();
        return;
    }
    case TokenKind::Assign:
        expr.type = rightType;
        return;
    case TokenKind::Equal:
    case TokenKind::NotEqual:
    case TokenKind::Less:
    case TokenKind::Greater:
    case TokenKind::LessEqual:
    case TokenKind::GreaterEqual:
        expr.type = TypeFactory::booleanType();
        return;
    case TokenKind::And:
    case TokenKind::Or:
        expr.type = TypeFactory::anyType();
        return;
    case TokenKind::Concat:
        if (isString(leftType) && isString(rightType)) {
            expr.type = TypeFactory::stringType();
            return;
        }
        throw error("Type error: concat requires string or number types");
    default:
        throw error("typeCheck not implemented for BinOpExpr");
    }
}

void TypeChecker::visit(MemberAccessExpr& expr) {
    expr.object->accept(*this);
    expr.type = TypeFactory::anyType();
}

void TypeChecker::visit(MethodAccessExpr& expr) {
    expr.object->accept(*this);
    expr.type = TypeFactory::anyType();
}

void TypeChecker::visit(FunCallExpr& expr) {
    expr.callee->accept(*this);
    for (auto& arg : expr.args) {
        arg->accept(*this);
    }
    expr.type = TypeFactory::anyType();
}

// Statement implementations

void TypeChecker::visit(FunDecl& stmt) { throw error("typeCheck not implemented for FunDecl"); }

void TypeChecker::visit(VarDecl& stmt) {
    stmt.init_expr->accept(*this);
    env.define(stmt.name, stmt.init_expr->type);
}

void TypeChecker::visit(VarDecls& stmt) {
    for (auto& decl : stmt.decls) {
        decl->accept(*this);
    }
}

void TypeChecker::visit(IfStmt& stmt) { stmt.condition->accept(*this); }

void TypeChecker::visit(ReturnStmt& stmt) {
    for (auto& val : stmt.return_values) {
        val->accept(*this);
    }
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
}
