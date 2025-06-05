#include "db/exceptions.h"
#include "parse/token_stream.h"

TokenStream::TokenStream(std::string data)
    : base_string(std::move(data)), stream(base_string) {}


bool is_whitespace(char chr) {
    return chr == ' ' || chr == '\t' || chr == '\n';
}

void TokenStream::ignore_whitespace() {
    while (!stream_empty() && is_whitespace(peek())) {
        get();
    }
}

bool TokenStream::empty() {
    return peek_token().get_type() == TokenType::Empty;
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
    return isalpha(c) || c == '_';
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

const std::string& TokenStream::peek_token(TokenType type) {
    const Token& res = peek_token();
    if(res.get_type() != type){
        throw InvalidQuery("Invalid token : " + res.get_value());
    }
    return res.get_value();
}

Token TokenStream::get_token() {
    load_next_token();
    
    Token result = std::move(next_token.value());
    next_token.reset();
    return result;
}

std::string TokenStream::get_token(TokenType type){
    Token token = peek_token();
    if(token.get_type() != type){
        throw InvalidQuery("Invalid token : " + token.get_value());
    }
    return get_token().get_value();
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
    if(c != quote){
        throw InvalidQuery("Unclosed string literal");
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
    std::string result = std::string(1, get());
    
    if((result == "<" || result == ">") && peek() == '='){
        result += get();
    }
    if(result == "<" && peek() == '>'){
        result += get();
    }
    
    return {TokenType::SpecialChar, result};
}

void TokenStream::ignore_token(const Token& token) {
    Token next = peek_token();
    
    if(token != next){
        throw InvalidQuery("Expected token " + token.get_value() + ", got " + next.get_value());
    }
    
    get_token();
}

void TokenStream::ignore_token(const std::string& token) {
    Token next = peek_token();
    
    if(!next.like(token)){
        throw InvalidQuery("Expected token " + token + ", got " + next.get_value());
    }
    
    get_token();
}

bool TokenStream::try_ignore_token(const Token& token) {
    Token next = peek_token();
    
    if(token != next){
        return false;
    }
    
    get_token();
    return true;
}

bool TokenStream::try_ignore_token(const std::string& token) {
    Token next = peek_token();
    
    if(!next.like(token)){
        return false;
    }
    
    get_token();
    return true;
}

void TokenStream::assert_end() {
    if(!empty()){
        throw InvalidQuery("Expected end of input, got " + peek_token().get_value());
    }
}
