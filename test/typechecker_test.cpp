#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/typechecker.h"
#include "../src/typed_ast_printer.h"
#include "./utils.h"
#include <catch2/catch_test_macros.hpp>

static std::string typecheck_and_print(const std::string& code) {
    auto tokens = Lexer::tokenize(code);
    Parser parser(tokens);
    auto prog = parser.parse();
    TypeChecker typechecker;
    typechecker.typeCheck(prog);
    TypedAstPrinter printer;
    return printer.print(prog);
}

TEST_CASE("Arithmetic") {
    std::string code = "local a = 1 + 2";
    std::string expected = "(var-decl a (+ <number> 1 <number> 2 <number>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("ArithmeticWithAny") {
    std::string code = "local a = x + 2";
    std::string expected = "(var-decl a (+ <number> (var x <any>) 2 <number>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Comparison") {
    std::string code = "local a = 1 > 2";
    std::string expected = "(var-decl a (> <boolean> 1 <number> 2 <number>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("ComparisonWithAny") {
    std::string code = "local a = x == y";
    std::string expected = "(var-decl a (== <boolean> (var x <any>) (var y <any>)))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Logical") {
    std::string code = "local a = x and y";
    std::string expected = "(var-decl a (and <any> (var x <any>) (var y <any>)))\n";
    REQUIRE(typecheck_and_print(code) == expected);

    std::string code2 = "local b = true or 1";
    std::string expected2 = "(var-decl b (or <boolean | number> true <boolean> 1 <number>))\n";
    REQUIRE(typecheck_and_print(code2) == expected2);
}

TEST_CASE("ThrowsTypeErrorOnArithmetic") {
    std::string code = "local b = a + \"hello\"";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);
}

TEST_CASE("function call") {
    std::string code = R"(
function add(a, b)
    return a + b
end

local result = add(2, 3)
)";
    std::string expected = R"(
(fun add (params a b) (block (return (+ <number> (var a <any>) (var b <any>)))))
(var-decl result (call <any> (var add <(any, any) -> any>) 2 <number> 3 <number>))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}
// Table literal tests

TEST_CASE("Empty table is empty record") {
    std::string code = "local t = {}";
    std::string expected = "(var-decl t (table <{  }>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Homogeneous array literal") {
    std::string code = "local arr = {1, 2, 3}";
    std::string expected =
        "(var-decl arr (table (array 1 <number> 2 <number> 3 <number>) <number[]>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Heterogeneous array literal creates union") {
    std::string code = "local arr = {1, \"hello\", true}";
    std::string expected = "(var-decl arr (table (array 1 <number> 'hello' <string> true "
                           "<boolean>) <number | string | boolean[]>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Record table literal") {
    std::string code = "local obj = {x = 1, y = 2}";
    // Note: map keys are sorted alphabetically in std::map
    std::string expected =
        "(var-decl obj (table (map (x 1 <number>) (y 2 <number>)) <{ x: number, y: number }>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Record table with mixed value types") {
    std::string code = "local obj = {name = \"alice\", age = 30}";
    // Note: map keys are sorted alphabetically in std::map
    std::string expected = "(var-decl obj (table (map (age 30 <number>) (name 'alice' <string>)) "
                           "<{ age: number, name: string }>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Nested table literal") {
    std::string code = "local obj = {inner = {1, 2, 3}}";
    std::string expected = "(var-decl obj (table (map (inner (table (array 1 <number> 2 <number> 3 "
                           "<number>) <number[]>))) <{ inner: number[] }>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Mixed table literal is forbidden") {
    std::string code = "local t = {1, 2, a = 3}";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);
}

TEST_CASE("Member access on record") {
    std::string code = "local obj = {x = 10}\nlocal a = obj.x";
    std::string expected = R"(
(var-decl obj (table (map (x 10 <number>)) <{ x: number }>))
(var-decl a (MemberAccess <number> (var obj <{ x: number }>) (var x <any>)))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Member access on unknown field throws") {
    std::string code = R"(
local obj = {x = 10}
local a = obj.z
)";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);
}