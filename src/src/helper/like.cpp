#include "helper/like.h"

#include <regex>

static std::string escape(char c){
    static const std::string special_regex_chars = "([\\^$.|?*+(){}])";
    
    std::string result;
    if(std::count(special_regex_chars.begin(), special_regex_chars.end(), c)){
        result += "\\";
    }
    result += c;
    return result;
}

static std::string convert_to_regex(const std::string& pattern){
    std::string result="^";
    for(auto c : pattern){
        if(c == '%'){
            result += ".*";
        }else if(c == '_'){
            result += ".";
        }else{
            result += escape(c);
        }
    }
    return result + "$";
}

bool is_like(const std::string& value, const std::string& pattern){
    std::string regex_pattern = convert_to_regex(pattern);
    std::regex regex(regex_pattern);
    return std::regex_match(value, regex);
}
