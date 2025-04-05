#ifndef TYPE_H
#define TYPE_H

#include "db/cell.h"
#include "helper/string.h"
#include "db/exceptions.h"

Cell::DataType string_to_type(const std::string& str) {
    std::string upper = to_upper(str);
    if (str == "INT") return Cell::DataType::Int;
    if (str == "FLOAT") return Cell::DataType::Float;
    if (str == "STRING") return Cell::DataType::String;
    if (str == "CHAR") return Cell::DataType::Char;
    throw ParsingError("Invalid type");
}

#endif