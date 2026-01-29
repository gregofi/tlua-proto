#include "../src/typed_ast_printer.h"
#include "type.h"
#include <cassert>
#include <format>
namespace {
const char* tokenKindToSymbol(TokenKind kind) {
    switch (kind) {
    case TokenKind::Plus:
        return "+";
    case TokenKind::Minus:
        return "-";
    case TokenKind::Star:
        return "*";
    case TokenKind::Slash:
        return "/";
    case TokenKind::Equal:
        return "==";
    case TokenKind::NotEqual:
        return "~=";
    case TokenKind::Less:
        return "<";
    case TokenKind::Greater:
        return ">";
    case TokenKind::LessEqual:
        return "<=";
    case TokenKind::GreaterEqual:
        return ">=";
    case TokenKind::And:
        return "and";
    case TokenKind::Or:
        return "or";
    case TokenKind::Not:
        return "not";
    case TokenKind::Concat:
        return "..";
    case TokenKind::Assign:
        return "=";
    default:
        return tokenKindToStr(kind);
    }
}
} // namespace
std::string TypedAstPrinter::print(Program& program) {
    result.clear();
    for (auto& stmt : program.statements) {
        stmt->accept(*this);
        result += "\n";
    }
    return result;
}

std::string TypedAstPrinter::print(Expr& expr) {
    result.clear();
    expr.accept(*this);
    return result;
}

std::string TypedAstPrinter::print(Stmt& stmt) {
    result.clear();
    stmt.accept(*this);
    return result;
}

void TypedAstPrinter::parenthesize(const std::string& name, const std::vector<Expr*>& exprs) {
    result += "(";
    result += name;
    for (auto& expr : exprs) {
        result += " ";
        expr->accept(*this);
    }
    result += ")";
}

void TypedAstPrinter::parenthesize(const std::string& name,
                                   const std::vector<std::unique_ptr<Expr>>& exprs) {
    result += "(";
    result += name;
    for (auto& expr : exprs) {
        result += " ";
        expr->accept(*this);
    }
    result += ")";
}

void TypedAstPrinter::visit(StringExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for StringExpr");
    result += std::format("'{}' <{}>", expr.val, expr.type->toString());
}

void TypedAstPrinter::visit(NumberExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for NumberExpr");
    result += std::format("{} <{}>", expr.val, expr.type->toString());
}

void TypedAstPrinter::visit(NilExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for NilExpr");
    result += std::format("nil <{}>", expr.type->toString());
}

void TypedAstPrinter::visit(VarExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for VarExpr");
    result += std::format("(var {} <{}>)", expr.name, expr.type->toString());
}

void TypedAstPrinter::visit(UnaryOpExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for UnaryOpExpr");
    result += std::format("({} <{}> ", tokenKindToSymbol(expr.op), expr.type->toString());
    expr.right->accept(*this);
    result += ")";
}

void TypedAstPrinter::visit(BinOpExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for BinOpExpr");
    result += std::format("({} <{}> ", tokenKindToSymbol(expr.op), expr.type->toString());
    expr.left->accept(*this);
    result += " ";
    expr.right->accept(*this);
    result += ")";
}

void TypedAstPrinter::visit(MemberAccessExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for MemberAccessExpr");
    result += std::format("(get <{}> ", expr.type->toString());
    expr.object->accept(*this);
    result += std::format(" {})", expr.member_name);
}

void TypedAstPrinter::visit(MethodAccessExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for MethodAccessExpr");
    result += std::format("(method <{}> ", expr.type->toString());
    expr.object->accept(*this);
    result += std::format(" {})", expr.method_name);
}

void TypedAstPrinter::visit(FunCallExpr& expr) {
    assert(expr.type != nullptr && "Type not inferred for FunCallExpr");
    result += std::format("(call <{}> ", expr.type->toString());
    expr.callee->accept(*this);
    for (auto& arg : expr.args) {
        result += " ";
        arg->accept(*this);
    }
    result += ")";
}

void TypedAstPrinter::visit(FunDecl& stmt) {
    result += "(fun " + stmt.name;
    result += " (params";
    for (const auto& param : stmt.params) {
        result += " " + param;
    }
    result += ") ";
    for (auto& body_stmt : stmt.body) {
        body_stmt->accept(*this);
    }
    result += ")";
}

void TypedAstPrinter::visit(VarDecl& stmt) {
    result += "(var-decl " + stmt.name + " ";
    stmt.init_expr->accept(*this);
    result += ")";
}

void TypedAstPrinter::visit(VarDecls& stmt) {
    result += "(var-decls ";
    for (size_t i = 0; i < stmt.decls.size(); ++i) {
        stmt.decls[i]->accept(*this);
        if (i < stmt.decls.size() - 1) {
            result += " ";
        }
    }
    result += ")";
}

void TypedAstPrinter::visit(IfStmt& stmt) {
    result += "(if ";
    stmt.condition->accept(*this);
    result += " then ";
    for (auto& then_stmt : stmt.then_body) {
        then_stmt->accept(*this);
    }
    for (auto& else_stmt : stmt.else_body) {
        result += " else ";
        else_stmt->accept(*this);
    }
    result += ")";
}

void TypedAstPrinter::visit(ReturnStmt& stmt) {
    result += "(return";
    for (auto& val : stmt.return_values) {
        result += " ";
        val->accept(*this);
    }
    result += ")";
}

void TypedAstPrinter::visit(BlockStmt& stmt) {
    result += "(block";
    for (auto& s : stmt.statements) {
        result += " ";
        s->accept(*this);
    }
    result += ")";
}

void TypedAstPrinter::visit(FunCallStmt& stmt) { stmt.call->accept(*this); }

void TypedAstPrinter::visit(AssignStmt& stmt) {
    result += "(assign ";
    stmt.left->accept(*this);
    result += " ";
    stmt.right->accept(*this);
    result += ")";
}
