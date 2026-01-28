#include "environment.h"

void Environment::pushScope() { scopes.emplace_back(); }

void Environment::popScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

void Environment::define(const std::string& name, Type* type) {
    if (scopes.empty()) {
        // Implicitly create global scope if needed
        pushScope();
    }
    scopes.back()[name] = type;
}

Type* Environment::lookup(const std::string& name) const {
    // Search from innermost to outermost scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr;
}
