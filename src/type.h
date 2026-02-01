#pragma once
#include "utils.h"
#include <format>
#include <map>
#include <memory>
#include <string>
#include <vector>
enum class TypeKind {
    // Primitive Types
    Number,
    String,
    Boolean,
    Nil,

    Unknown,
    Any,

    // Composite Types
    Array,  // string[]
    Table,  // { x: number, y: string }
    Record, // { [key: Type]: Type }

    Function, // (Type, Type) -> Type

    Union, // string | number
};

class Type {
  public:
    explicit Type(TypeKind kind) : kind(kind) {}

    virtual ~Type() = default;

    TypeKind getKind() const { return kind; }

    virtual std::string toString() const = 0;

  private:
    TypeKind kind;
};

class BasicType : public Type {
  public:
    static Type* numberType() {
        static BasicType instance(TypeKind::Number);
        return &instance;
    }

    static Type* stringType() {
        static BasicType instance(TypeKind::String);
        return &instance;
    }

    static Type* booleanType() {
        static BasicType instance(TypeKind::Boolean);
        return &instance;
    }

    static Type* nilType() {
        static BasicType instance(TypeKind::Nil);
        return &instance;
    }

    static Type* unknownType() {
        static BasicType instance(TypeKind::Unknown);
        return &instance;
    }

    static Type* anyType() {
        static BasicType instance(TypeKind::Any);
        return &instance;
    }

    std::string toString() const override {
        switch (getKind()) {
        case TypeKind::Number:
            return "number";
        case TypeKind::String:
            return "string";
        case TypeKind::Boolean:
            return "boolean";
        case TypeKind::Nil:
            return "nil";
        case TypeKind::Unknown:
            return "unknown";
        case TypeKind::Any:
            return "any";
        default:
            return "<invalid-basic-type>";
        }
    }

  private:
    explicit BasicType(TypeKind kind) : Type(kind) {}
};

class FunctionType : public Type {
  public:
    FunctionType(std::vector<Type*> param_types, Type* return_type)
        : Type(TypeKind::Function), paramTypes(std::move(param_types)), returnType(return_type) {}

    const std::vector<Type*>& getParamTypes() const { return paramTypes; }

    Type* getReturnType() const { return returnType; }

    std::string toString() const override {
        auto toStrings =
            std::ranges::views::transform(paramTypes, [](auto&& t) { return t->toString(); });
        return std::format("({}) -> {}", join(toStrings, ", "), returnType->toString());
    }

  private:
    std::vector<Type*> paramTypes;
    Type* returnType;
};

class UnionType : public Type {
  public:
    explicit UnionType(std::vector<Type*> types) : Type(TypeKind::Union), types(std::move(types)) {}
    const std::vector<Type*>& getTypes() const { return types; }

    std::string toString() const override {
        auto toStrings =
            std::ranges::views::transform(types, [](auto&& t) { return t->toString(); });
        return join(toStrings, " | ");
    }

  private:
    std::vector<Type*> types;
};

class ArrayType : public Type {
  public:
    explicit ArrayType(Type* element_type) : Type(TypeKind::Array), elementType(element_type) {}
    Type* getElementType() const { return elementType; }

    std::string toString() const override { return std::format("{}[]", elementType->toString()); }

  private:
    Type* elementType;
};

class TableType : public Type {
  public:
    explicit TableType(std::map<std::string, Type*> fields)
        : Type(TypeKind::Table), fields(std::move(fields)) {}
    const std::map<std::string, Type*>& getFields() const { return fields; }

    std::string toString() const override {
        auto fieldStrings = std::ranges::views::transform(fields, [](auto&& field) {
            return std::format("{}: {}", field.first, field.second->toString());
        });
        return std::format("{{ {} }}", join(fieldStrings, ", "));
    }

  private:
    std::map<std::string, Type*> fields;
};

class RecordType : public Type {
  public:
    RecordType(Type* key_type, Type* value_type)
        : Type(TypeKind::Record), keyType(key_type), valueType(value_type) {}
    Type* getKeyType() const { return keyType; }
    Type* getValueType() const { return valueType; }

    std::string toString() const override {
        return std::format("{{ [{}]: {} }}", keyType->toString(), valueType->toString());
    }

  private:
    Type* keyType;
    Type* valueType;
};

// Type utilities
bool isSameType(Type* a, Type* b);
bool isSubtype(Type* sub, Type* super);
std::string typeToString(Type* type);

/// Unify multiple types into a single type.
/// If all types are the same, returns that type.
/// Otherwise, creates a union type.
Type* unifyTypes(std::vector<Type*> types);

// Type factory - owns all complex types
class TypeFactory {
  public:
    static TypeFactory& instance();

    // Primitive types (singletons, not owned by factory)
    static Type* numberType() { return BasicType::numberType(); }
    static Type* stringType() { return BasicType::stringType(); }
    static Type* booleanType() { return BasicType::booleanType(); }
    static Type* nilType() { return BasicType::nilType(); }
    static Type* unknownType() { return BasicType::unknownType(); }
    static Type* anyType() { return BasicType::anyType(); }

    // Complex type factories (owned by factory)
    Type* createFunctionType(std::vector<Type*> paramTypes, Type* returnType);
    Type* createArrayType(Type* elementType);
    Type* createTableType(std::map<std::string, Type*> fields);
    Type* createRecordType(Type* keyType, Type* valueType);
    Type* createUnionType(std::vector<Type*> types);

  private:
    TypeFactory() = default;
    std::vector<std::unique_ptr<Type>> types;
};
