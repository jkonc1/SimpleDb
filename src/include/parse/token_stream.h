#ifndef TOKEN_STREAM_H
#define TOKEN_STREAM_H

#include <string>
#include <sstream>
#include <optional>

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
    
    /**
     * @brief Check if token value matches string case insensitively
     */
    bool like(const std::string& what) const{
        if(value.size() != what.size()) {
            return false;
        }
        for (size_t i = 0; i < value.size(); ++i) {
            if (tolower(value[i]) != tolower(what[i])) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Get the raw representation of the token
     */
    std::string get_raw() const{
        if(type == TokenType::String){
            return "\"" + value + "\"";
        }
        return value;
    }
};

/**
 * @brief Stream of tokens for parsing
 */
class TokenStream {
public:
    /**
     * @brief Construct TokenStream from a string
     */
    TokenStream(std::string data);
    
    /**
     * @brief Get the next token from the stream
     */
    Token get_token();
    
    /**
     * @brief Get the next token of a specific type
     * @throws InvalidQuery if the token doesn't match the expected type
     */
    std::string get_token(TokenType type);
    
    /**
     * @brief Peek at the next token without consuming it
     */
    const Token& peek_token();
    
    /**
     * @brief Peek at the next token of a specific type
     * @throws InvalidQuery if the token doesn't match the expected type
     */
    const std::string& peek_token(TokenType type);
    
    /**
     * @brief Skip the next token if it matches the expected token
     * @throws InvalidQuery if the token doesn't match
     */
    void ignore_token(const Token& token);
    
    /**
     * @brief Skip the next token if it matches the expected string
     * @details Case insensitive
     * @throws InvalidQuery if the token doesn't match
     */
    void ignore_token(const std::string& token);
    
    /**
     * @brief Try to skip the next token if it matches
     * @return True if the token was matched and ignored, false otherwise
     */
    bool try_ignore_token(const Token& token);
    
    /**
     * @brief Try to skip the next token if it matches
     * @details Case insensitive
     * @return True if the token was matched and ignored, false otherwise
     */
    bool try_ignore_token(const std::string& token);
    
    /**
     * @brief Check if the stream is empty
     */
    bool empty();
    
    /**
     * @brief Assert that the stream is at the end
     * @throws InvalidQuery if there are more tokens in the stream
     */
    void assert_end();

private:
    /**
     * @brief Peek at the next character
     */
    char peek();
    
    /**
     * @brief Get the next character
     */
    char get();
    
    /**
     * @brief Skip whitespace characters
     */
    void ignore_whitespace();
    
    /**
     * @brief Parse an identifier
     */
    Token get_identifier();
    
    /**
     * @brief Parse a number
     */
    Token get_number();
    
    /**
     * @brief Parse a string
     */
    Token get_string();
    
    /**
     * @brief Parse a special character
     */
    Token get_special_char();
    
    /**
     * @brief Check if the underlying stream is empty
     */
    bool stream_empty();
    
    /**
     * @brief Load the next token into the buffer
     */
    void load_next_token();
    
    std::string base_string;
    std::optional<Token> next_token;
    std::stringstream stream;
};

#endif