#include "db/exceptions.h"
#include "doctest.h"
#include "parse/token_stream.h"

TEST_CASE("TokenStream"){
    TokenStream stream("  SELECT * FROM table_ WHERE Name='Peter Bucman' | 7.2 = \"11\"");

    DOCTEST_CHECK_EQ(stream.get_token(), Token{TokenType::Identifier, "SELECT"});
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::SpecialChar), "*");
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::Identifier), "FROM");
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::Identifier), "table_");
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::Identifier), "WHERE");
    
    DOCTEST_CHECK_THROWS_AS(stream.get_token(TokenType::SpecialChar), InvalidQuery);
    
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::Identifier), "Name");
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::SpecialChar), "=");
    
    DOCTEST_CHECK_EQ(stream.peek_token().value, "Peter Bucman");
    
    DOCTEST_CHECK_THROWS_AS(stream.ignore_token("="), InvalidQuery);
    
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::String), "Peter Bucman");
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::SpecialChar), "|");
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::Number), "7.2");
    
    stream.ignore_token("=");
    
    DOCTEST_CHECK_EQ(stream.empty(), false);
    
    DOCTEST_CHECK_EQ(stream.get_token(TokenType::String), "11");
    DOCTEST_CHECK_EQ(stream.empty(), true);
    DOCTEST_CHECK_EQ(stream.get_token(), Token{TokenType::Empty, ""});
}
