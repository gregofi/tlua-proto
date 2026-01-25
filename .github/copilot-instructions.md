# AI Assistant Instructions for tlua-proto

## Project Overview
This is a typed Lua compiler/interpreter project written in C++.

## C++ Coding Standards

### Modern C++ Version
- Use modern C++ features up to **C++23**
- Leverage the latest standard library features where applicable

### String Formatting
- **Use `std::format`** for string formatting and concatenation instead of streams
- Example: `std::format("Value: {}", x)` instead of stringstream operations

### Ranges and Iterators
- **Prefer ranges** over traditional `begin()`/`end()` iterator pairs where possible
- Use range-based algorithms from `<ranges>` when applicable

### Functional Programming
- **Prefer `std::accumulate`, `std::transform`** and other standard algorithms instead of manual for loops
- Use range algorithms for cleaner, more expressive code

### Lambda Expressions
- **Use `auto&& x`** in lambda parameters where possible instead of writing explicit types
- Example: `[](auto&& item) { ... }` instead of `[](const MyType& item) { ... }`

### Constructors
- Use member initializer lists for constructors
- Pass by value and use `std::move` for movable types to avoid unnecessary copies

## General C++ Best Practices
- Follow RAII principles for resource management
- Prefer smart pointers over raw pointers, but there are exceptions! For example, Type is used as simple pointer.
- Use `constexpr` and `const` appropriately
- Leverage structured bindings where appropriate
