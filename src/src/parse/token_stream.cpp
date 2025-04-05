#include "db/exceptions.h"
#include "parse/token_stream.h"


TokenStream::TokenStream(std::string data)
    : base_string(data), stream(base_string) {}


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

bool TokenStream::empty() {
    return peek_token().type == TokenType::Empty;
}

char TokenStream::get() {
    return stream.get();
}

char TokenStream::peek() {
    return stream.peek();
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

void TokenStream::load_next_token() {
    if(next_token.has_value()){
        return;
    }
    
    ignore_whitespace();
    
    char c = peek();
    
    if (c == EOF){
        next_token =  {TokenType::Empty, ""};
    }
    else if(starts_string(c)){
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
    Token token = get_token();
    if(token.type != type){
        throw InvalidQuery("Invalid token type");
    }
    return token.value;
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

void TokenStream::ignore_token(Token token) {
    Token next = get_token();
    
    if(token != next){
        throw InvalidQuery("Expected token " + token.value + ", got " + next.value);
    }
}

void TokenStream::ignore_token(std::string token) {
    Token next = get_token();
    
    if(token != next.value){
        throw InvalidQuery("Expected token " + token + ", got " + next.value);
    }
}
