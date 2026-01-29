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

void TypeChecker::visit(BooleanExpr& expr) { expr.type = TypeFactory::booleanType(); }

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

    switch (expr.op) {
    case TokenKind::Minus:
        if (!isNumber(expr.right->type)) {
            throw error("Type error: unary minus requires a number type");
        }
        expr.type = expr.right->type;
        break;
    case TokenKind::Not:
        expr.type = TypeFactory::booleanType();
        break;
    default:
        throw error("Unknown unary operator in type checker");
    }
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
        expr.type = TypeFactory::booleanType();
        return;
    case TokenKind::Less:
    case TokenKind::Greater:
    case TokenKind::LessEqual:
    case TokenKind::GreaterEqual:
        if ((isNumber(leftType) && isNumber(rightType)) ||
            (isString(leftType) && isString(rightType))) {
            expr.type = TypeFactory::booleanType();
            return;
        }
        throw error("Type error: comparison requires number or string types");
    case TokenKind::And:
    case TokenKind::Or:
        // The result of logical ops can be any of the operand types => union
        expr.type = TypeFactory::instance().createUnionType({leftType, rightType});
        return;
    case TokenKind::Concat:
        if (isString(leftType) && isString(rightType)) {
            expr.type = TypeFactory::stringType();
            return;
        }
        throw error("Type error: concat requires string or number types");
    default:
        throw error("Unknown binary operator in type checker");
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
    // check that its function
    auto calleeType = expr.callee->type;
    if (calleeType->getKind() != TypeKind::Function) {
        throw error("Type error: trying to call a non-function type");
    }

    // parameter arity
    auto funType = static_cast<FunctionType*>(calleeType);
    if (funType->getParamTypes().size() != expr.args.size()) {
        throw error("Type error: function called with incorrect number of arguments");
    }

    for (auto& arg : expr.args) {
        arg->accept(*this);
    }

    // actual parameter types
    for (size_t i = 0; i < expr.args.size(); ++i) {
        Type* expectedType = funType->getParamTypes()[i];
        Type* actualType = expr.args[i]->type;
        if (!isSubtype(actualType, expectedType)) {
            throw error("Type error: function argument type mismatch");
        }
    }

    expr.type = funType->getReturnType();
}

// Statement implementations

void TypeChecker::visit(FunDecl& stmt) {
    std::vector<Type*> paramTypes;
    for (const auto& _ : stmt.params) {
        paramTypes.push_back(TypeFactory::anyType());
    }
    Type* returnType = TypeFactory::anyType();
    Type* funcType = TypeFactory::instance().createFunctionType(paramTypes, returnType);
    env.define(stmt.name, funcType);

    env.pushScope();
    for (size_t i = 0; i < stmt.params.size(); ++i) {
        env.define(stmt.params[i], paramTypes[i]);
    }
    stmt.body->accept(*this);
    env.popScope();
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
    stmt.then_branch->accept(*this);
    if (stmt.else_branch) {
        stmt.else_branch->accept(*this);
    }
}

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
