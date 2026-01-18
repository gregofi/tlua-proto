#include <catch2/catch_test_macros.hpp>
#include "../src/lexer.h"
#include "../src/parser.h"

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
    REQUIRE_NOTHROW(parse(
        "local x = 10\n"
        "function add(a, b) return a end\n"
    ));
}

TEST_CASE("parse if statements") {
    REQUIRE_NOTHROW(parse(
        "if x then\n"
        "  return 1\n"
        "else\n"
        "  return 0\n"
        "end\n"
    ));
}

TEST_CASE("parse nested if statements") {
    REQUIRE_NOTHROW(parse(
        "if x then\n"
        "  if y then\n"
        "    return 1\n"
        "  else\n"
        "    return 2\n"
        "  end\n"
        "else\n"
        "  return 0\n"
        "end\n"
    ));
}

TEST_CASE("throw error on invalid syntax") {
    REQUIRE_THROWS_AS(parse("local = 10\n"), ParseError);
    REQUIRE_THROWS_AS(parse("function add(a b) return a end\n"), ParseError);
    REQUIRE_THROWS_AS(parse("if x then return 1\n"), ParseError);
}

TEST_CASE("parse return statements with multiple values") {
    REQUIRE_NOTHROW(parse(
        "function foo()\n"
        "  return 1, 2, 3\n"
        "end\n"
    ));
}

// TODO
TEST_CASE("parse function declarations with dot and colon", "[!mayfail]") {
    REQUIRE_NOTHROW(parse(
        "function obj:method(a)\n"
        "  return a\n"
        "end\n"
    ));
    REQUIRE_NOTHROW(parse(
        "function obj.method(a)\n"
        "  return a\n"
        "end\n"
    ));
}

TEST_CASE("binary expressions parsing") {
    REQUIRE_NOTHROW(parse("local result = a + b"));
    REQUIRE_NOTHROW(parse("local result = x * (y + z)"));
    REQUIRE_NOTHROW(parse("local result = x * (y + z)"));
    REQUIRE_NOTHROW(parse("local result = a == b"));
}

TEST_CASE("parse member access expressions", "[!mayfail]") {
    REQUIRE_NOTHROW(parse("local value = obj.field"));
    REQUIRE_NOTHROW(parse("local value = obj.nested.field") );
    REQUIRE_NOTHROW(parse("local value = obj:method()"));
    REQUIRE_NOTHROW(parse("local value = obj.method()"));
    REQUIRE_NOTHROW(parse("local value = obj:nested:method()") );
    REQUIRE_NOTHROW(parse("local value = foo().bar + 1") );
}

TEST_CASE("parse fibonacci program", "[!mayfail]") {
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
    REQUIRE_NOTHROW(parse(fibProgram));
}
