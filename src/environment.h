#pragma once
#include "type.h"
#include <map>
#include <string>
#include <vector>

class Environment {
  public:
    Environment() = default;

    // Enter a new scope (for blocks, functions, etc.)
    void pushScope();

    // Exit the current scope
    void popScope();

    // Bind a variable name to a type
    void define(const std::string& name, Type* type);

    // Look up a variable's type (searches from innermost to outermost scope)
    // Returns nullptr if not found
    Type* lookup(const std::string& name) const;

  private:
    // Stack of scopes, each scope is a map from name -> type
    // scopes[0] is global, scopes[scopes.size()-1] is current
    std::vector<std::map<std::string, Type*>> scopes;
};
