#include "type.h"

bool isSameType(Type* a, Type* b) {
    if (a == b) {
        return true;
    }
    if (a == nullptr || b == nullptr) {
        return false;
    }

    if (a->getKind() != b->getKind()) {
        return false;
    }

    switch (a->getKind()) {
    case TypeKind::Number:
    case TypeKind::String:
    case TypeKind::Boolean:
    case TypeKind::Nil:
    case TypeKind::Unknown:
    case TypeKind::Any:
        // Primitives are singletons, pointer equality is sufficient
        return a == b;

    case TypeKind::Array: {
        auto* arr1 = static_cast<ArrayType*>(a);
        auto* arr2 = static_cast<ArrayType*>(b);
        return isSameType(arr1->getElementType(), arr2->getElementType());
    }

    case TypeKind::Function: {
        auto* fn1 = static_cast<FunctionType*>(a);
        auto* fn2 = static_cast<FunctionType*>(b);

        if (fn1->getParamTypes().size() != fn2->getParamTypes().size()) {
            return false;
        }

        for (size_t i = 0; i < fn1->getParamTypes().size(); ++i) {
            if (!isSameType(fn1->getParamTypes()[i], fn2->getParamTypes()[i])) {
                return false;
            }
        }

        return isSameType(fn1->getReturnType(), fn2->getReturnType());
    }

    case TypeKind::Union: {
        auto* union1 = static_cast<UnionType*>(a);
        auto* union2 = static_cast<UnionType*>(b);

        if (union1->getTypes().size() != union2->getTypes().size()) {
            return false;
        }

        // Check if all types in union1 are in union2 (order independent)
        for (auto* type1 : union1->getTypes()) {
            bool found = false;
            for (auto* type2 : union2->getTypes()) {
                if (isSameType(type1, type2)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }

    case TypeKind::Table: {
        auto* table1 = static_cast<TableType*>(a);
        auto* table2 = static_cast<TableType*>(b);

        if (table1->getFields().size() != table2->getFields().size()) {
            return false;
        }

        for (const auto& field1 : table1->getFields()) {
            bool found = false;
            for (const auto& field2 : table2->getFields()) {
                if (field1.key == field2.key && isSameType(field1.val, field2.val)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }

    case TypeKind::Record: {
        auto* rec1 = static_cast<RecordType*>(a);
        auto* rec2 = static_cast<RecordType*>(b);
        return isSameType(rec1->getKeyType(), rec2->getKeyType()) &&
               isSameType(rec1->getValueType(), rec2->getValueType());
    }
    }

    return false;
}

bool isSubtype(Type* sub, Type* super) {
    if (isSameType(sub, super)) {
        return true;
    }

    if (super == nullptr || sub == nullptr) {
        return false;
    }

    // Any is a supertype of everything
    if (super->getKind() == TypeKind::Any) {
        return true;
    }

    // Unknown is a subtype of everything (gradual typing)
    if (sub->getKind() == TypeKind::Unknown) {
        return true;
    }

    // Union types: sub is subtype of union if sub is subtype of any member
    if (super->getKind() == TypeKind::Union) {
        auto* unionType = static_cast<UnionType*>(super);
        for (auto* memberType : unionType->getTypes()) {
            if (isSubtype(sub, memberType)) {
                return true;
            }
        }
        return false;
    }

    // If sub is a union, all members must be subtypes of super
    if (sub->getKind() == TypeKind::Union) {
        auto* unionType = static_cast<UnionType*>(sub);
        for (auto* memberType : unionType->getTypes()) {
            if (!isSubtype(memberType, super)) {
                return false;
            }
        }
        return true;
    }

    return false;
}

std::string typeToString(Type* type) {
    if (type == nullptr) {
        return "<null>";
    }
    return type->toString();
}

TypeFactory& TypeFactory::instance() {
    static TypeFactory factory;
    return factory;
}

Type* TypeFactory::createFunctionType(std::vector<Type*> paramTypes, Type* returnType) {
    types.push_back(std::make_unique<FunctionType>(std::move(paramTypes), returnType));
    return types.back().get();
}

Type* TypeFactory::createArrayType(Type* elementType) {
    types.push_back(std::make_unique<ArrayType>(elementType));
    return types.back().get();
}

Type* TypeFactory::createTableType(std::vector<TableField> fields) {
    types.push_back(std::make_unique<TableType>(std::move(fields)));
    return types.back().get();
}

Type* TypeFactory::createRecordType(Type* keyType, Type* valueType) {
    types.push_back(std::make_unique<RecordType>(keyType, valueType));
    return types.back().get();
}

Type* TypeFactory::createUnionType(std::vector<Type*> types_list) {
    // If Any is in the union, the whole union is Any
    if (std::any_of(types_list.begin(), types_list.end(),
                    [](Type* t) { return t->getKind() == TypeKind::Any; })) {
        return anyType();
    }

    types.push_back(std::make_unique<UnionType>(std::move(types_list)));
    return types.back().get();
}
