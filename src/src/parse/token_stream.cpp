#include <net/if.h>
#include <parse/token_stream.h>

TokenStream::TokenStream(std::unique_ptr<std::istream> stream)
    : stream(std::move(stream)) {}


bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

void TokenStream::ignore_whitespace() {
    char c = peek();
    while (is_whitespace(c)) {
        get();
        c = peek();
    }
}

char TokenStream::get() {
    return stream->get();
}

char TokenStream::peek() {
    return stream->peek();
}

bool starts_identifier(char c) {
    return isalpha(c);
}

bool starts_number(char c) {
    return isdigit(c) || c == '.';
}

bool starts_string(char c) {
    return c == '"';
}

Token TokenStream::_get_token() {
    if (empty()){
        return {TokenType::Empty, ""};
    }
    
    char c = peek();
    if(starts_string(c)){
        return get_string();
    }
    
    if(starts_identifier(c)){
        return get_identifier();
    }
    
    if(starts_number(c)){
        return get_number();
    }
    
    return get_special_char();
}

Token TokenStream::get_token() {
    ignore_whitespace();
    
    Token result = _get_token();
    
    ignore_whitespace();
    return result;
}

Token TokenStream::get_number() {
    std::string value;
    char c = peek();
    while (isdigit(c) || c == '.') {
        value += get();
        c = peek();
    }
    return {TokenType::Number, value};
}

Token TokenStream::get_string() {
    std::string value;
    char c = get();
    while (c != '"') {
        value += c;
        c = get();
    }
    return {TokenType::String, value};
}

Token TokenStream::get_identifier() {
    std::string value;
    char c = peek();
    while (isalpha(c) || isdigit(c) || c == '_') {
        value += get();
        c = peek();
    }
    return {TokenType::Identifier, value};
}

Token TokenStream::get_special_char(){
    return {TokenType::SpecialChar, std::to_string(get())};
}
