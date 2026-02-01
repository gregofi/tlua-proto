#include "../src/lexer.h"
#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("should tokenize length operator") {
    std::string source = "#arr";
    auto tokens = Lexer::tokenize(source);
    REQUIRE(tokens.size() == 3); // #, identifier, eof
    REQUIRE(tokens[0].kind == TokenKind::Length);
    REQUIRE(tokens[0].lexeme == "#");
    REQUIRE(tokens[1].kind == TokenKind::Identifier);
    REQUIRE(tokens[1].lexeme == "arr");
    REQUIRE(tokens[2].kind == TokenKind::Eof);
}

TEST_CASE("should tokenize bracket indexing") {
    std::string source = "arr[1]";
    auto tokens = Lexer::tokenize(source);
    REQUIRE(tokens.size() == 5); // identifier, [, number, ], eof
    REQUIRE(tokens[0].kind == TokenKind::Identifier);
    REQUIRE(tokens[0].lexeme == "arr");
    REQUIRE(tokens[1].kind == TokenKind::LBracket);
    REQUIRE(tokens[1].lexeme == "[");
    REQUIRE(tokens[2].kind == TokenKind::Number);
    REQUIRE(tokens[2].lexeme == "1");
    REQUIRE(tokens[3].kind == TokenKind::RBracket);
    REQUIRE(tokens[3].lexeme == "]");
    REQUIRE(tokens[4].kind == TokenKind::Eof);
}
