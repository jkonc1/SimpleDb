#ifndef TOKEN_STREAM_H
#define TOKEN_STREAM_H

#include <string>
#include <sstream>

enum class TokenType {
    Identifier,
    Number,
    SpecialChar,
    String,
    Empty
};

struct Token{
    TokenType type;
    std::string value;
    
    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }
};

class TokenStream {
public:
    TokenStream(std::string string);
    
    Token get_token();
    std::string get_token(TokenType type);
    
    const Token& peek_token();
    
    
    void ignore_token(Token token);
    void ignore_token(std::string token);
    
    bool empty();
    
    void assert_end();

private:
    char peek();
    char get();
    
    void ignore_whitespace();
    
    Token get_identifier();
    Token get_number();
    Token get_string();
    Token get_special_char();
    
    bool stream_empty();
    
    void load_next_token();
    
    std::string base_string;
    std::optional<Token> next_token;
    std::stringstream stream;
};

#endif