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
    
    bool like(const std::string& what) const{
        for (size_t i = 0; i < value.size(); ++i) {
            if (tolower(value[i]) != tolower(what[i])) {
                return false;
            }
        }
        return true;
    }
};

class TokenStream {
public:
    TokenStream(std::string data);
    
    Token get_token();
    std::string get_token(TokenType type);
    
    const Token& peek_token();
    const std::string& peek_token(TokenType type);
    
    void ignore_token(const Token& token);
    void ignore_token(const std::string& token);
    
    bool try_ignore_token(const Token& token);
    bool try_ignore_token(const std::string& token);
    
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