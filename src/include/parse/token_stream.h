#ifndef TOKEN_STREAM_H
#define TOKEN_STREAM_H

#include <memory>
#include <string>
#include <istream>

enum class TokenType {
    Identifier,
    Number,
    SpecialChar,
    String,
    Empty
};

using Token = std::pair<TokenType, std::string>;

class TokenStream {
public:
    TokenStream(std::unique_ptr<std::istream> stream);
    
    Token get_token();
    
    void ignore_whitespace();
    
    bool empty() const;

private:
    char peek();
    char get();
    
    Token get_identifier();
    Token get_number();
    Token get_string();
    Token get_special_char();
    
    Token _get_token();
    
    std::unique_ptr<std::istream> stream;
};

#endif