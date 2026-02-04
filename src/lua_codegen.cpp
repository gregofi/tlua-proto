#include "lua_codegen.h"
#include "utils.h"
#include <algorithm>
#include <format>
#include <vector>

namespace {
const char* tokenKindToLuaOperator(TokenKind kind) {
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
    case TokenKind::Length:
        return "#";
    case TokenKind::Concat:
        return "..";
    default:
        return tokenKindToStr(kind);
    }
}
} // namespace

std::string LuaCodegen::indent() const { return std::string(indent_level * 4, ' '); }

void LuaCodegen::newline() { result += "\n"; }

std::string LuaCodegen::generateExprString(Expr& expr) {
    std::string saved_result = std::move(result);
    int saved_indent = indent_level;
    result.clear();
    expr.accept(*this);
    std::string expr_result = std::move(result);
    result = std::move(saved_result);
    indent_level = saved_indent;
    return expr_result;
}

std::string LuaCodegen::generate(Program& program) {
    result.clear();
    indent_level = 0;
    for (size_t i = 0; i < program.statements.size(); ++i) {
        program.statements[i]->accept(*this);
        if (i < program.statements.size() - 1) {
            newline();
        }
    }
    return result;
}

std::string LuaCodegen::generate(Expr& expr) {
    result.clear();
    indent_level = 0;
    expr.accept(*this);
    return result;
}

std::string LuaCodegen::generate(Stmt& stmt) {
    result.clear();
    indent_level = 0;
    stmt.accept(*this);
    return result;
}

void LuaCodegen::visit(StringExpr& expr) {
    // Escape special characters in the string
    std::string escaped;
    for (char c : expr.val) {
        switch (c) {
        case '\n':
            escaped += "\\n";
            break;
        case '\t':
            escaped += "\\t";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        default:
            escaped += c;
        }
    }
    result += std::format("\"{}\"", escaped);
}

void LuaCodegen::visit(NumberExpr& expr) {
    // Format number without trailing zeros for integers
    if (expr.val == static_cast<int>(expr.val)) {
        result += std::format("{}", static_cast<int>(expr.val));
    } else {
        result += std::format("{}", expr.val);
    }
}

void LuaCodegen::visit(NilExpr& /*expr*/) { result += "nil"; }

void LuaCodegen::visit(BooleanExpr& expr) { result += expr.val ? "true" : "false"; }

void LuaCodegen::visit(TableExpr& expr) {
    result += "{";
    std::vector<std::string> elements;

    // Array elements
    for (const auto& element : expr.arrayPart) {
        elements.push_back(generateExprString(*element));
    }

    // Key-value pairs
    for (const auto& [key, value] : expr.mapPart) {
        std::string keyValStr = std::format("[{}] = {}", key, generateExprString(*value));
        elements.push_back(keyValStr);
    }

    result += join(elements, ", ");
    result += "}";
}

void LuaCodegen::visit(VarExpr& expr) { result += expr.name; }

void LuaCodegen::visit(UnaryOpExpr& expr) {
    auto op = tokenKindToLuaOperator(expr.op);
    result += op;
    // Add space after 'not' keyword
    if (expr.op == TokenKind::Not) {
        result += " ";
    }
    expr.right->accept(*this);
}

void LuaCodegen::visit(BinOpExpr& expr) {
    expr.left->accept(*this);
    result += std::format(" {} ", tokenKindToLuaOperator(expr.op));
    expr.right->accept(*this);
}

void LuaCodegen::visit(IndexExpr& expr) {
    expr.object->accept(*this);
    result += "[";
    expr.index->accept(*this);
    result += "]";
}

void LuaCodegen::visit(FunCallExpr& expr) {
    expr.callee->accept(*this);
    result += "(";
    std::vector<std::string> argStrings;
    std::transform(expr.args.begin(), expr.args.end(), std::back_inserter(argStrings),
                   [this](auto&& arg) { return generateExprString(*arg); });
    result += join(argStrings, ", ");
    result += ")";
}

void LuaCodegen::visit(FunDecl& stmt) {
    result += indent();
    if (stmt.local) {
        result += "local ";
    }
    result += "function ";

    if (stmt.thisName) {
        result += *stmt.thisName;
        if (stmt.method) {
            result += ":";
        } else {
            result += ".";
        }
    }
    result += stmt.name;

    result += "(";
    std::vector<std::string> params;
    std::transform(stmt.params.begin(), stmt.params.end(), std::back_inserter(params),
                   [](auto&& param) { return param.toString(); });
    result += join(params, ", ");
    result += ")";
    newline();

    ++indent_level;
    stmt.body->accept(*this);
    --indent_level;

    newline();
    result += indent();
    result += "end";
}

void LuaCodegen::visit(VarDecl& stmt) {
    result += indent();
    result += "local ";
    result += stmt.name;
    result += " = ";
    stmt.initExpr->accept(*this);
}

void LuaCodegen::visit(VarDecls& stmt) {
    result += indent();
    result += "local ";
    std::vector<std::string> names;
    std::transform(stmt.decls.begin(), stmt.decls.end(), std::back_inserter(names),
                   [](auto&& decl) { return decl->name; });
    result += join(names, ", ");
    result += " = ";
    std::vector<std::string> initExprs;
    std::transform(stmt.decls.begin(), stmt.decls.end(), std::back_inserter(initExprs),
                   [this](auto&& decl) { return generateExprString(*decl->initExpr); });
    result += join(initExprs, ", ");
}

void LuaCodegen::visit(IfStmt& stmt) {
    result += indent();
    result += "if ";
    stmt.condition->accept(*this);
    result += " then";
    newline();

    ++indent_level;
    stmt.then_branch->accept(*this);
    --indent_level;

    if (stmt.else_branch) {
        newline();
        result += indent();
        // Check if else branch is another IfStmt (elseif pattern)
        if (dynamic_cast<IfStmt*>(stmt.else_branch.get())) {
            result += "else";
            newline();
            ++indent_level;
            stmt.else_branch->accept(*this);
            --indent_level;
            newline();
            result += indent();
            result += "end";
        } else {
            result += "else";
            newline();
            ++indent_level;
            stmt.else_branch->accept(*this);
            --indent_level;
            newline();
            result += indent();
            result += "end";
        }
    } else {
        newline();
        result += indent();
        result += "end";
    }
}

void LuaCodegen::visit(ReturnStmt& stmt) {
    result += indent();
    result += "return";
    if (!stmt.return_values.empty()) {
        result += " ";
        std::vector<std::string> returnStrings;
        std::transform(stmt.return_values.begin(), stmt.return_values.end(),
                       std::back_inserter(returnStrings),
                       [this](auto&& val) { return generateExprString(*val); });
        result += join(returnStrings, ", ");
    }
}

void LuaCodegen::visit(BlockStmt& stmt) {
    for (size_t i = 0; i < stmt.statements.size(); ++i) {
        stmt.statements[i]->accept(*this);
        if (i < stmt.statements.size() - 1) {
            newline();
        }
    }
}

void LuaCodegen::visit(FunCallStmt& stmt) {
    result += indent();
    stmt.call->accept(*this);
}

void LuaCodegen::visit(AssignStmt& stmt) {
    result += indent();
    stmt.left->accept(*this);
    result += " = ";
    stmt.right->accept(*this);
}
