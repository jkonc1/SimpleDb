#include "parse/keywords.h"
#include "helper/string.h"

#include <vector>

static const std::vector<std::string> keyword_list = {
    "SELECT",
    "DELETE",
    "FROM",
    "WHERE",
    "GROUP",
    "BY",
    "HAVING",
    "INSERT",
    "INTO",
    "VALUES",
    "DROP",
    "TABLE",
    "CREATE",
    "ALL",
    "DISTINCT",
    "MAX",
    "MIN",
    "AVG",
    "COUNT",
    "SUM",
    "BETWEEN",
    "LIKE",
    "NULL",
    "AND",
    "OR",
    "NOT",
    "ANY",
    "ALL",
    "EXISTS",
    "IN"
};

bool is_keyword(const std::string &word){
    std::string upper = to_upper(word);
    for(const auto& keyword : keyword_list){
        if(upper == keyword){
            return true;
        }
    }
    
    return false;
}