#include "../src/environment.h"
#include "../src/type.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Environment: basic define and lookup") {
    Environment env;
    env.pushScope();

    env.define("x", BasicType::numberType());
    Type* found = env.lookup("x");

    REQUIRE(found != nullptr);
    REQUIRE(isSameType(found, BasicType::numberType()));
}

TEST_CASE("Environment: lookup returns nullptr for undefined variable") {
    Environment env;
    env.pushScope();

    Type* found = env.lookup("undefined");

    REQUIRE(found == nullptr);
}

TEST_CASE("Environment: multiple variables in same scope") {
    Environment env;
    env.pushScope();

    env.define("x", BasicType::numberType());
    env.define("y", BasicType::stringType());
    env.define("z", BasicType::booleanType());

    REQUIRE(isSameType(env.lookup("x"), BasicType::numberType()));
    REQUIRE(isSameType(env.lookup("y"), BasicType::stringType()));
    REQUIRE(isSameType(env.lookup("z"), BasicType::booleanType()));
}

TEST_CASE("Environment: lookup across scopes (inner to outer)") {
    Environment env;

    env.pushScope(); // outer scope
    env.define("x", BasicType::numberType());

    env.pushScope(); // inner scope
    env.define("y", BasicType::stringType());

    // Can see both
    REQUIRE(isSameType(env.lookup("x"), BasicType::numberType()));
    REQUIRE(isSameType(env.lookup("y"), BasicType::stringType()));

    env.popScope();

    // Outer scope can only see x
    REQUIRE(isSameType(env.lookup("x"), BasicType::numberType()));
    REQUIRE(env.lookup("y") == nullptr);
}

TEST_CASE("Environment: shadowing in nested scopes") {
    Environment env;

    env.pushScope(); // outer scope
    env.define("x", BasicType::numberType());

    env.pushScope();                          // inner scope
    env.define("x", BasicType::stringType()); // shadow with different type

    // Inner scope sees shadowed version
    REQUIRE(isSameType(env.lookup("x"), BasicType::stringType()));

    env.popScope();

    // Outer scope still has original
    REQUIRE(isSameType(env.lookup("x"), BasicType::numberType()));
}

TEST_CASE("Environment: pop scope removes variables") {
    Environment env;

    env.pushScope();
    env.define("x", BasicType::numberType());

    REQUIRE(env.lookup("x") != nullptr);

    env.popScope();

    REQUIRE(env.lookup("x") == nullptr);
}