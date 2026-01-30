#include "../src/lexer.h"
#include "../src/lua_codegen.h"
#include "../src/parser.h"
#include "../src/typechecker.h"
#include "./utils.h"
#include <catch2/catch_test_macros.hpp>

static std::string generate_lua(const std::string& code) {
    auto tokens = Lexer::tokenize(code);
    Parser parser(tokens);
    auto prog = parser.parse();
    TypeChecker typechecker;
    typechecker.typeCheck(prog);
    LuaCodegen codegen;
    return codegen.generate(prog);
}

TEST_CASE("Codegen simple variable declaration") {
    std::string code = "local a = 42";
    std::string expected = "local a = 42";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen string literal") {
    std::string code = R"(local s = "hello")";
    std::string expected = R"(local s = "hello")";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen arithmetic expression") {
    std::string code = "local x = 1 + 2 * 3";
    std::string expected = "local x = 1 + 2 * 3";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen function declaration") {
    std::string code = R"(
function add(a, b)
    return a + b
end
)";
    std::string expected = R"(function add(a, b)
    return a + b
end)";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen local function") {
    std::string code = R"(
local function greet(name)
    return name
end
)";
    std::string expected = R"(local function greet(name)
    return name
end)";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen function call") {
    // Need to define the function first
    std::string code = R"(
function foo(a, b, c)
    return a
end
foo(1, 2, 3)
)";
    std::string expected = R"(function foo(a, b, c)
    return a
end
foo(1, 2, 3))";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen if statement") {
    std::string code = R"(
function test()
    if true then
        return 1
    end
end
)";
    std::string expected = R"(function test()
    if true then
        return 1
    end
end)";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen if-else statement") {
    std::string code = R"(
function test()
    if true then
        return 1
    else
        return 0
    end
end
)";
    std::string expected = R"(function test()
    if true then
        return 1
    else
        return 0
    end
end)";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen boolean literals") {
    std::string code = "local t = true";
    REQUIRE(generate_lua(code) == "local t = true");

    std::string code2 = "local f = false";
    REQUIRE(generate_lua(code2) == "local f = false");
}

TEST_CASE("Codegen nil literal") {
    std::string code = "local n = nil";
    std::string expected = "local n = nil";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen unary not") {
    std::string code = "local x = not true";
    std::string expected = "local x = not true";
    REQUIRE(generate_lua(code) == expected);
}

TEST_CASE("Codegen comparison operators") {
    REQUIRE(generate_lua("local a = 1 < 2") == "local a = 1 < 2");
    REQUIRE(generate_lua("local a = 1 > 2") == "local a = 1 > 2");
    REQUIRE(generate_lua("local a = 1 <= 2") == "local a = 1 <= 2");
    REQUIRE(generate_lua("local a = 1 >= 2") == "local a = 1 >= 2");
}

TEST_CASE("Codegen logical operators") {
    REQUIRE(generate_lua("local a = true and false") == "local a = true and false");
    REQUIRE(generate_lua("local a = true or false") == "local a = true or false");
}
