#ifndef TYPE_H
#define TYPE_H

#include "db/cell.h"
#include "helper/string.h"
#include "db/exceptions.h"

/**
 * @brief Convert a string to a Cell::DataType
 * @throws ParsingError if the string doesn't represent a valid type
 */
inline Cell::DataType string_to_type(const std::string& str) {
    std::string upper = to_upper(str);
    if (upper == "INT") { 
        return Cell::DataType::Int;
    }
    if (upper == "FLOAT") {
        return Cell::DataType::Float;
    }
    if (upper == "STRING") { 
        return Cell::DataType::String;
    }
    if (upper == "CHAR") { 
        return Cell::DataType::Char;
    }
    throw ParsingError("Invalid type");
}

#endif