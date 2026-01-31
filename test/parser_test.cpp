#include "../src/lexer.h"
#include "../src/parser.h"
#include "./utils.h"
#include <catch2/catch_test_macros.hpp>

const Program parse(const std::string& source) {
    auto tokens = Lexer::tokenize(source);
    Parser parser(tokens);
    return parser.parse();
}

TEST_CASE("parse one declaration program") {
    REQUIRE_NOTHROW(parse("local x = 10\n"));
    REQUIRE_NOTHROW(parse("function add(a) return a end\n"));
}

TEST_CASE("parse multiple declarations") {
    REQUIRE_NOTHROW(parse("local x = 10\n"
                          "function add(a, b) return a end\n"));
}

TEST_CASE("parse if statements") {
    REQUIRE_NOTHROW(parse("if x then\n"
                          "  return 1\n"
                          "else\n"
                          "  return 0\n"
                          "end\n"));
}

TEST_CASE("parse nested if statements") {
    REQUIRE_NOTHROW(parse("if x then\n"
                          "  if y then\n"
                          "    return 1\n"
                          "  else\n"
                          "    return 2\n"
                          "  end\n"
                          "else\n"
                          "  return 0\n"
                          "end\n"));
}

TEST_CASE("parse if-elseif-else statements") {
    auto prog = parse("if x == 1 then\n"
                      "  return 1\n"
                      "elseif x == 2 then\n"
                      "  return 2\n"
                      "else\n"
                      "  return 0\n"
                      "end\n");
    auto expected = R"(
(if
    (Equal (var x) (number 1))
    (then (block (return (number 1))))
    (else
        (if
            (Equal (var x) (number 2))
            (then (block (return (number 2))))
            (else (block (return (number 0)))))))
)";
    REQUIRE(normalize(prog.statements.at(0)->toSExpr()) == normalize(expected));
}

TEST_CASE("throw error on invalid syntax") {
    REQUIRE_THROWS_AS(parse("local = 10\n"), ParseError);
    REQUIRE_THROWS_AS(parse("function add(a b) return a end\n"), ParseError);
    REQUIRE_THROWS_AS(parse("if x then return 1\n"), ParseError);
}

TEST_CASE("parse return statements with multiple values") {
    REQUIRE_NOTHROW(parse("function foo()\n"
                          "  return 1, 2, 3\n"
                          "end\n"));
}

TEST_CASE("binary expressions parsing") {
    REQUIRE_NOTHROW(parse("local result = a + b"));
    REQUIRE_NOTHROW(parse("local result = x * (y + z)"));
    REQUIRE_NOTHROW(parse("local result = x * (y + z)"));
    REQUIRE_NOTHROW(parse("local result = a == b"));
}

TEST_CASE("unary expressions parsing") {
    REQUIRE_NOTHROW(parse("local result = -x"));
    REQUIRE_NOTHROW(parse("local result = not flag"));
}

TEST_CASE("parse function calls") {
    REQUIRE_NOTHROW(parse("local result = foo()"));
    REQUIRE_NOTHROW(parse("local result = foo(1)"));
    REQUIRE_NOTHROW(parse("local result = foo(1, 2)"));
    REQUIRE_NOTHROW(parse("local result = (foo)(x, y)"));
}

TEST_CASE("parse function calls as statements") {
    REQUIRE_NOTHROW(parse("foo()"));
    REQUIRE_NOTHROW(parse("foo(1, 2)"));
}

TEST_CASE("parse member access expressions") {
    REQUIRE_NOTHROW(parse("local value = obj.field"));
    REQUIRE_NOTHROW(parse("local value = obj.nested.field"));
    REQUIRE_NOTHROW(parse("local value = obj:method()"));
    REQUIRE_NOTHROW(parse("local value = obj.method()"));
    REQUIRE_NOTHROW(parse("local value = obj:nested:method()"));
    REQUIRE_NOTHROW(parse("local value = foo().bar + 1"));
}

TEST_CASE("parse fibonacci program") {
    const std::string fibProgram = R"(
function fib(n)
    if n == 0 then
        return 0
    elseif n == 1 then
        return 1
    else
        return fib(n - 1) + fib(n - 2)
    end
end

local result = fib(10)
print(result)
)";
    auto expected = R"(
(fun global fib (n)
    (block (if (Equal (var n) (number 0))
        (then (block (return (number 0))))
        (else
            (if (Equal (var n) (number 1))
                (then (block (return (number 1))))
                (else (block (return (Plus
                                (call (var fib) (Minus (var n) (number 1)))
                                (call (var fib) (Minus (var n) (number 2))))))))))))
(var-decl result (call (var fib) (number 10)))
(call (var print) (var result))
)";
    auto prog = parse(fibProgram);
    std::string progSExpr;
    for (const auto& stmt : prog.statements) {
        progSExpr += stmt->toSExpr() + " ";
    }

    REQUIRE(normalize(progSExpr) == normalize(expected));
}

TEST_CASE("parse variable declarations with no initializer") {
    auto prog = parse("local x\n");
    auto expected = R"(
(var-decl x (nil))
)";
    REQUIRE(normalize(prog.statements.at(0)->toSExpr()) == normalize(expected));
}

TEST_CASE("parse variable assignments") {
    REQUIRE_NOTHROW(parse("x = 10\n"));
    REQUIRE_NOTHROW(parse("obj.field = 42\n"));
}

TEST_CASE("parse table expressions") {
    REQUIRE_NOTHROW(parse("local t = {}\n"));
    REQUIRE_NOTHROW(parse("local t = {1, 2, 3}\n"));
    REQUIRE_NOTHROW(parse("local t = {a = 1, b = 2}\n"));
    REQUIRE_NOTHROW(parse("local t = {1, 2, a = 3, b = 4}\n"));
}
