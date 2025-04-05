#include "helper/string.h"

#include <algorithm>

std::string to_upper(const std::string& str){
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    return upper;
}

std::string to_lower(const std::string& str){
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}