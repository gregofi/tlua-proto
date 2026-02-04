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
    std::string expected = "(var-decl a <number> (+ <number> 1 <number> 2 <number>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("ArithmeticWithAny") {
    std::string code = "local a = x + 2";
    std::string expected = "(var-decl a <number> (+ <number> (var x <any>) 2 <number>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Comparison") {
    std::string code = "local a = 1 > 2";
    std::string expected = "(var-decl a <boolean> (> <boolean> 1 <number> 2 <number>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("ComparisonWithAny") {
    std::string code = "local a = x == y";
    std::string expected = "(var-decl a <boolean> (== <boolean> (var x <any>) (var y <any>)))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Logical") {
    std::string code = "local a = x and y";
    std::string expected = "(var-decl a <any> (and <any> (var x <any>) (var y <any>)))\n";
    REQUIRE(typecheck_and_print(code) == expected);

    std::string code2 = "local b = true or 1";
    std::string expected2 =
        "(var-decl b <boolean | number> (or <boolean | number> true <boolean> 1 <number>))\n";
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
(fun add <(any, any) -> any> (params a b) (block (return (+ <number> (var a <any>) (var b <any>)))))
(var-decl result <any> (call <any> (var add <(any, any) -> any>) 2 <number> 3 <number>))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}
// Table literal tests

TEST_CASE("Empty table is empty record") {
    std::string code = "local t = {}";
    std::string expected = "(var-decl t <{  }> (table <{  }>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Homogeneous array literal") {
    std::string code = "local arr = {1, 2, 3}";
    std::string expected =
        "(var-decl arr <number[]> (table (array 1 <number> 2 <number> 3 <number>) <number[]>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Heterogeneous array literal creates union") {
    std::string code = "local arr = {1, \"hello\", true}";
    std::string expected = "(var-decl arr <number | string | boolean[]> (table (array 1 <number> "
                           "'hello' <string> true "
                           "<boolean>) <number | string | boolean[]>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Record table literal") {
    std::string code = "local obj = {x = 1, y = 2}";
    // Note: map keys are sorted alphabetically in std::map
    std::string expected = "(var-decl obj <{ x: number, y: number }> (table (map (x 1 <number>) (y "
                           "2 <number>)) <{ x: number, y: number }>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Record table with mixed value types") {
    std::string code = "local obj = {name = \"alice\", age = 30}";
    // Note: map keys are sorted alphabetically in std::map
    std::string expected = "(var-decl obj <{ age: number, name: string }> (table (map (age 30 "
                           "<number>) (name 'alice' <string>)) "
                           "<{ age: number, name: string }>))\n";
    REQUIRE(typecheck_and_print(code) == expected);
}

TEST_CASE("Nested table literal") {
    std::string code = "local obj = {inner = {1, 2, 3}}";
    std::string expected = "(var-decl obj <{ inner: number[] }> (table (map (inner (table (array 1 "
                           "<number> 2 <number> 3 "
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
(var-decl obj <{ x: number }> (table (map (x 10 <number>)) <{ x: number }>))
(var-decl a <number> (MemberAccess <number> (var obj <{ x: number }>) (var x <any>)))
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

TEST_CASE("Length operator on array") {
    std::string code = "local arr = {1, 2, 3}\nlocal len = #arr";
    std::string expected = R"(
(var-decl arr <number[]> (table (array 1 <number> 2 <number> 3 <number>) <number[]>))
(var-decl len <number> (# <number> (var arr <number[]>)))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Length operator on nested expression") {
    std::string code = "local len = #{1, 2, 3}";
    std::string expected = R"(
(var-decl len <number> (# <number> (table (array 1 <number> 2 <number> 3 <number>) <number[]>)))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Length operator on non-array throws") {
    std::string code = "local len = #42";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);
}

TEST_CASE("Length operator on record throws") {
    std::string code = "local obj = {x = 10}\nlocal len = #obj";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);
}

TEST_CASE("Array indexing returns element type") {
    std::string code = "local arr = {1, 2, 3}\nlocal val = arr[1]";
    std::string expected = R"(
(var-decl arr <number[]> (table (array 1 <number> 2 <number> 3 <number>) <number[]>))
(var-decl val <number> ([] <number> (var arr <number[]>) 1 <number>))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Array indexing with variable index") {
    std::string code = "local arr = {1, 2, 3}\nlocal i = 1\nlocal val = arr[i]";
    std::string expected = R"(
(var-decl arr <number[]> (table (array 1 <number> 2 <number> 3 <number>) <number[]>))
(var-decl i <number> 1 <number>)
(var-decl val <number> ([] <number> (var arr <number[]>) (var i <number>)))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Nested array indexing") {
    std::string code = "local arr = {{1, 2}}\nlocal val = arr[1][2]";
    std::string expected = R"(
(var-decl arr <number[][]> (table (array (table (array 1 <number> 2 <number>) <number[]>)) <number[][]>))
(var-decl val <number> ([] <number> ([] <number[]> (var arr <number[][]>) 1 <number>) 2 <number>))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Array indexing with non-number throws") {
    std::string code = "local arr = {1, 2, 3}\nlocal val = arr[\"key\"]";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);
}

TEST_CASE("Indexing non-array throws") {
    std::string code = "local val = 42[1]";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);
}

TEST_CASE("Table indexing returns any") {
    std::string code = "local tbl = {x = 10}\nlocal val = tbl[\"x\"]";
    std::string expected = R"(
(var-decl tbl <{ x: number }> (table (map (x 10 <number>)) <{ x: number }>))
(var-decl val <any> ([] <any> (var tbl <{ x: number }>) 'x' <string>))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Type annotations - basic types in variables") {
    std::string code = R"(
local n: number = 42
local s: string = "hello"
local b: boolean = true
local x: nil = nil
)";
    std::string expected = R"(
(var-decl n <number> 42 <number>)
(var-decl s <string> 'hello' <string>)
(var-decl b <boolean> true <boolean>)
(var-decl x <nil> nil <nil>)
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Type annotations - function with typed parameters and return") {
    std::string code = R"(
function add(x: number, y: number) -> number
    return x + y
end

local result = add(2, 3)
)";
    std::string expected = R"(
(fun add <(number, number) -> number> (params x y) (block (return (+ <number> (var x <number>) (var y <number>)))))
(var-decl result <number> (call <number> (var add <(number, number) -> number>) 2 <number> 3 <number>))
)";
    REQUIRE(normalize(typecheck_and_print(code)) == normalize(expected));
}

TEST_CASE("Type annotations - type mismatch should error") {
    std::string code = "local x: number = true";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);

    std::string code2 = "local s: string = 42";
    REQUIRE_THROWS_AS(typecheck_and_print(code2), TypeCheckError);

    std::string code3 = "local b: boolean = \"hello\"";
    REQUIRE_THROWS_AS(typecheck_and_print(code3), TypeCheckError);
}

TEST_CASE("Type annotations - function parameter type mismatch should error") {
    std::string code = R"(
function add(x: number, y: number) -> number
    return x + y
end

local result = add("hello", 5)
)";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);

    std::string code2 = R"(
function greet(name: string) -> string
    return name
end

local result = greet(42)
)";
    REQUIRE_THROWS_AS(typecheck_and_print(code2), TypeCheckError);
}

TEST_CASE("Type annotations - return type mismatch should error") {
    std::string code = R"(
function foo() -> number
    return "hello"
end
)";
    REQUIRE_THROWS_AS(typecheck_and_print(code), TypeCheckError);

    std::string code2 = R"(
function bar() -> string
    return 42
end
)";
    REQUIRE_THROWS_AS(typecheck_and_print(code2), TypeCheckError);
}
