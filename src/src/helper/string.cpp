#include "helper/string.h"

#include <algorithm>

std::string to_upper(const std::string& str){
    std::string upper = str;
    for (auto& c : upper) {
        c = toupper(c);
    }
    return upper;
}

std::string to_lower(const std::string& str){
    std::string lower = str;
    for (auto& c : lower) {
        c = tolower(c);
    }
    return lower;
}