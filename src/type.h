#pragma once
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
    Array, // string[]
    Table, // { x: number, y: string }
    Record, // { [key: Type]: Type }

    Function, // (Type, Type) -> Type

    Union, // string | number
};

class Type {
public:
    explicit Type(TypeKind kind) : kind(kind) {}

    virtual ~Type() = default;
private:
    TypeKind kind;
};

class PrimitiveType : public Type {
public:
    Type* numberType() {
        static PrimitiveType instance(TypeKind::Number);
        return &instance;
    }

    Type* stringType() {
        static PrimitiveType instance(TypeKind::String);
        return &instance;
    }

    Type* booleanType() {
        static PrimitiveType instance(TypeKind::Boolean);
        return &instance;
    }

    Type* nilType() {
        static PrimitiveType instance(TypeKind::Nil);
        return &instance;
    }

    Type* unknownType() {
        static PrimitiveType instance(TypeKind::Unknown);
        return &instance;
    }

    Type* anyType() {
        static PrimitiveType instance(TypeKind::Any);
        return &instance;
    }
protected:
    PrimitiveType(TypeKind kind) : Type(kind) {}
};

class FunctionType : public Type {
public:
    FunctionType(std::vector<Type*> param_types, Type* return_type)
        : Type(TypeKind::Function),
          paramTypes(std::move(param_types)),
          returnType(return_type) {}

    const std::vector<Type*>& getParamTypes() const {
        return paramTypes;
    }

    Type* getReturnType() const {
        return returnType;
    }
private:
    std::vector<Type*> paramTypes;
    Type* returnType;
};

class UnionType : public Type {
public:
    explicit UnionType(std::vector<Type*> types)
        : Type(TypeKind::Union), types(std::move(types)) {}
    const std::vector<Type*>& getTypes() const {
        return types;
    }
private:
    std::vector<Type*> types;
};

class ArrayType : public Type {
public:
    explicit ArrayType(Type* element_type)
        : Type(TypeKind::Array), elementType(element_type) {}
    Type* getElementType() const {
        return elementType;
    }
private:
    Type* elementType;
};

struct TableField {
    std::string key;
    Type* val;
};

class TableType : public Type {
public:
    explicit TableType(std::vector<TableField> fields)
        : Type(TypeKind::Table), fields(std::move(fields)) {}
    const std::vector<TableField>& getFields() const
    {
        return fields;
    }
private:
    std::vector<TableField> fields;
};

class RecordType : public Type {
public:
    RecordType(Type* key_type, Type* value_type)
        : Type(TypeKind::Record), keyType(key_type), valueType(value_type) {}
    Type* getKeyType() const {
        return keyType;
    }
    Type* getValueType() const {
        return valueType;
    }
private:
    Type* keyType;
    Type* valueType;
};

