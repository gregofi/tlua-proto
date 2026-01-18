#include <catch2/catch_test_macros.hpp>
#include "../src/lexer.h"

TEST_CASE("should be able to tokenize simple source") {
    std::string source = "local x = 10";
    auto tokens = Lexer::tokenize(source);
    REQUIRE(tokens.size() == 5); // local, identifier, =, number, eof
    REQUIRE(tokens[0].kind == TokenKind::Local);
    REQUIRE(tokens[1].kind == TokenKind::Identifier);
    REQUIRE(tokens[1].lexeme == "x");
    REQUIRE(tokens[2].kind == TokenKind::Assign);
    REQUIRE(tokens[3].kind == TokenKind::Number);
    REQUIRE(tokens[3].lexeme == "10");
    REQUIRE(tokens[4].kind == TokenKind::Eof);
}
