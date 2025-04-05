#include "db/exceptions.h"
#include "parse/token_stream.h"


TokenStream::TokenStream(std::string data)
    : base_string(data), stream(base_string) {}


bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

void TokenStream::ignore_whitespace() {
    while (!stream_empty() && is_whitespace(peek())) {
        get();
    }
}

bool TokenStream::empty() {
    return peek_token().type == TokenType::Empty;
}

char TokenStream::get() {
    return stream.get();
}

char TokenStream::peek() {
    return stream.peek();
}

bool TokenStream::stream_empty() {
    return peek() == EOF;
}

bool starts_identifier(char c) {
    return isalpha(c);
}

bool starts_number(char c) {
    return isdigit(c) || c == '.';
}

bool starts_string(char c) {
    return c == '"' || c == '\'';
}

void TokenStream::load_next_token() {
    if(next_token.has_value()){
        return;
    }
    
    ignore_whitespace();
    
    
    if (stream_empty()){
        next_token = {TokenType::Empty, ""};
        return;
    }
    
    char c = peek();
    if(starts_string(c)){
        next_token =  get_string();
    }
    else if(starts_identifier(c)){
        next_token = get_identifier();
    }
    else if(starts_number(c)){
        next_token = get_number();
    }
    else{
        next_token = get_special_char();
    }
    
    ignore_whitespace();
}

const Token& TokenStream::peek_token() {
    load_next_token();
    return next_token.value();
}

Token TokenStream::get_token() {
    load_next_token();
    
    Token result = std::move(next_token.value());
    next_token.reset();
    return result;
}

std::string TokenStream::get_token(TokenType type){
    Token token = peek_token();
    if(token.type != type){
        throw InvalidQuery("Invalid token type");
    }
    return get_token().value;
}

Token TokenStream::get_number() {
    std::string value;
    char c = peek();
    while (!stream_empty() && (isdigit(c) || c == '.')) {
        value += get();
        c = peek();
    }
    return {TokenType::Number, value};
}

Token TokenStream::get_string() {
    std::string value;
    char quote = get();
    
    char c = get();
    while (!stream_empty() && c != quote) {
        value += c;
        c = get();
    }
    return {TokenType::String, value};
}

Token TokenStream::get_identifier() {
    std::string value;
    char c = peek();
    while (!stream_empty() && (isalpha(c) || isdigit(c) || c == '_')) {
        value += get();
        c = peek();
    }
    return {TokenType::Identifier, value};
}

Token TokenStream::get_special_char(){
    return {TokenType::SpecialChar, std::string(1, get())};
}

void TokenStream::ignore_token(Token token) {
    Token next = peek_token();
    
    if(token != next){
        throw InvalidQuery("Expected token " + token.value + ", got " + next.value);
    }
    
    get_token();
}

void TokenStream::ignore_token(std::string token) {
    Token next = peek_token();
    
    if(token != next.value){
        throw InvalidQuery("Expected token " + token + ", got " + next.value);
    }
    
    get_token();
}
