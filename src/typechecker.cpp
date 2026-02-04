#include "typechecker.h"

namespace {
bool isAny(Type* type) { return type->getKind() == TypeKind::Any; }

bool isNumber(Type* type) { return isAny(type) || type == TypeFactory::numberType(); }

bool isString(Type* type) { return isAny(type) || type == TypeFactory::stringType(); }
} // namespace

TypeChecker::TypeChecker() {
    env.pushScope(); // global scope
}

void TypeChecker::visit(StringExpr& expr) { expr.type = TypeFactory::stringType(); }

void TypeChecker::visit(NumberExpr& expr) { expr.type = TypeFactory::numberType(); }

void TypeChecker::visit(NilExpr& expr) { expr.type = TypeFactory::nilType(); }

void TypeChecker::visit(BooleanExpr& expr) { expr.type = TypeFactory::booleanType(); }

void TypeChecker::visit(TableExpr& expr) {
    // Mixed tables are forbidden: either pure array or pure record
    bool hasArrayPart = !expr.arrayPart.empty();
    bool hasMapPart = !expr.mapPart.empty();

    if (hasArrayPart && hasMapPart) {
        throw error("Type error: mixed table literals are not allowed. "
                    "Use either array syntax {1, 2, 3} or record syntax {a = 1, b = 2}");
    }

    if (hasArrayPart) {
        // Infer array type with unified element type
        std::vector<Type*> elementTypes;
        elementTypes.reserve(expr.arrayPart.size());
        for (auto& elem : expr.arrayPart) {
            elem->accept(*this);
            elementTypes.push_back(elem->type);
        }
        Type* elementType = unifyTypes(std::move(elementTypes));
        expr.type = TypeFactory::instance().createArrayType(elementType);
    } else if (hasMapPart) {
        // Infer record/table type with named fields
        std::map<std::string, Type*> fields;
        for (auto& [key, value] : expr.mapPart) {
            value->accept(*this);
            fields[key] = value->type;
        }
        expr.type = TypeFactory::instance().createTableType(std::move(fields));
    } else {
        // Empty table {} is an empty record
        expr.type = TypeFactory::instance().createTableType({});
    }
}

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
    case TokenKind::Length:
        if (expr.right->type->getKind() != TypeKind::Array && !isAny(expr.right->type)) {
            throw error(std::format("Type error: length operator requires array type, got {}",
                                    expr.right->type->toString()));
        }
        expr.type = TypeFactory::numberType();
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
    case TokenKind::MemberAccess: {
        // Member access: left must be a table, right must be a VarExpr with the field name
        if (leftType->getKind() == TypeKind::Any) {
            expr.type = TypeFactory::anyType();
            return;
        }
        if (leftType->getKind() != TypeKind::Table) {
            throw error(std::format("Type error: cannot access member on non-table type {}",
                                    leftType->toString()));
        }
        auto* tableType = static_cast<TableType*>(leftType);
        // The right side is parsed as a VarExpr containing the field name
        auto* varExpr = dynamic_cast<VarExpr*>(expr.right.get());
        if (!varExpr) {
            throw error("Type error: member access requires an identifier");
        }
        auto it = tableType->getFields().find(varExpr->name);
        if (it != tableType->getFields().end()) {
            expr.type = it->second;
            return;
        }
        throw error(std::format("Type error: field '{}' does not exist on type {}", varExpr->name,
                                leftType->toString()));
    }
    case TokenKind::MethodAccess:
        // Method access returns any for now
        expr.type = TypeFactory::anyType();
        return;
    default:
        throw error("Unknown binary operator in type checker");
    }
}

void TypeChecker::visit(IndexExpr& expr) {
    expr.object->accept(*this);
    expr.index->accept(*this);

    Type* objType = expr.object->type;

    // Any type propagates
    if (objType->getKind() == TypeKind::Any) {
        expr.type = TypeFactory::anyType();
        return;
    }

    // Array indexing: arr[number] -> element type
    if (objType->getKind() == TypeKind::Array) {
        if (!isNumber(expr.index->type) && !isAny(expr.index->type)) {
            throw error(std::format("Type error: array index must be number, got {}",
                                    expr.index->type->toString()));
        }
        auto* arrayType = static_cast<ArrayType*>(objType);
        expr.type = arrayType->getElementType();
        return;
    }

    // Table (record) indexing: return any for dynamic access
    // This could be done better (compile-time evaluation), but
    // for this prototype we keep it simple.
    // We also want to be backward compatible with Lua, so by default
    // we are not strict -> This behavior with any is required..
    if (objType->getKind() == TypeKind::Table) {
        expr.type = TypeFactory::anyType();
        return;
    }

    throw error(
        std::format("Type error: cannot index non-array/table type {}", objType->toString()));
}

void TypeChecker::visit(FunCallExpr& expr) {
    expr.callee->accept(*this);
    auto calleeType = expr.callee->type;
    if (calleeType->getKind() == TypeKind::Any) {
        expr.type = TypeFactory::anyType();
        return;
    }

    // check that its function
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
    for (const auto& param : stmt.params) {
        if (param.typeAnnotation.has_value()) {
            paramTypes.push_back(resolveTypeAnnotation(param.typeAnnotation.value()));
        } else {
            paramTypes.push_back(TypeFactory::anyType());
        }
    }

    Type* returnType;
    if (stmt.returnTypeAnnotation.has_value()) {
        returnType = resolveTypeAnnotation(stmt.returnTypeAnnotation.value());
    } else {
        returnType = TypeFactory::anyType();
    }

    Type* funcType = TypeFactory::instance().createFunctionType(paramTypes, returnType);
    stmt.type = funcType;
    env.define(stmt.name, funcType);

    env.pushScope();
    for (size_t i = 0; i < stmt.params.size(); ++i) {
        env.define(stmt.params[i].name, paramTypes[i]);
    }

    // Track return type for validation
    Type* previousReturnType = currentFunctionReturnType;
    currentFunctionReturnType = returnType;

    stmt.body->accept(*this);

    currentFunctionReturnType = previousReturnType;
    env.popScope();
}

void TypeChecker::visit(VarDecl& stmt) {
    stmt.initExpr->accept(*this);

    if (stmt.typeAnnotation.has_value()) {
        Type* annotatedType = resolveTypeAnnotation(stmt.typeAnnotation.value());

        // Check if initializer type is compatible with annotated type
        if (!isSubtype(stmt.initExpr->type, annotatedType)) {
            throw error(std::format("Type mismatch: cannot assign {} to variable of type {}",
                                    stmt.initExpr->type->toString(), annotatedType->toString()));
        }

        stmt.type = annotatedType;
        env.define(stmt.name, annotatedType);
    } else {
        stmt.type = stmt.initExpr->type;
        env.define(stmt.name, stmt.initExpr->type);
    }
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

    // Validate return type if we're inside a function with a type annotation
    if (currentFunctionReturnType != nullptr &&
        currentFunctionReturnType->getKind() != TypeKind::Any) {

        if (stmt.return_values.size() == 1) {
            Type* returnedType = stmt.return_values[0]->type;
            if (!isSubtype(returnedType, currentFunctionReturnType)) {
                throw error(
                    std::format("Type mismatch: function returns {} but declared to return {}",
                                returnedType->toString(), currentFunctionReturnType->toString()));
            }
        }
        // TODO: Handle multiple return values
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

Type* TypeChecker::resolveTypeAnnotation(const TypeAnnotation& annotation) {
    return std::visit(
        overloaded{[](const BasicTypeAnnotation& basic) -> Type* {
                       switch (basic.kind) {
                       case BasicTypeAnnotation::Kind::Number:
                           return BasicType::numberType();
                       case BasicTypeAnnotation::Kind::String:
                           return BasicType::stringType();
                       case BasicTypeAnnotation::Kind::Boolean:
                           return BasicType::booleanType();
                       case BasicTypeAnnotation::Kind::Nil:
                           return BasicType::nilType();
                       }
                       return TypeFactory::unknownType();
                   },
                   [](const FunctionTypeAnnotation&) -> Type* {
                       // TODO: Implement function type annotations
                       throw TypeCheckError("Function type annotations not yet supported");
                   },
                   [](const TableTypeAnnotation&) -> Type* {
                       // TODO: Implement table type annotations
                       throw TypeCheckError("Table type annotations not yet supported");
                   },
                   [](const ArrayTypeAnnotation&) -> Type* {
                       // TODO: Implement array type annotations
                       throw TypeCheckError("Array type annotations not yet supported");
                   },
                   [](const UnionTypeAnnotation&) -> Type* {
                       // TODO: Implement union type annotations
                       throw TypeCheckError("Union type annotations not yet supported");
                   }},
        annotation.getVariant());
}
