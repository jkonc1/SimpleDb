#include "db/cell.h"
#include <stdexcept>

std::pair<Cell, Cell> Cell::promote_to_common(const Cell& a, const Cell& b) {
    // the promotion order is int -> float -> string -> null
    // char -> string
    
    if(a.is_null() || b.is_null()){
        return {Cell(), Cell()};
    }
    
    if(a.is_int() && b.is_int()){
        return {a, b};
    }
    
    if(a.is_char() && b.is_char()){
        return {a, b};
    }
    
    if(a.is_number() && b.is_number()){
        return {a.to_float(), b.to_float()};
    }
    
    return {a.to_string(), b.to_string()};
}

Cell Cell::to_float()const {
    if(is_int()){
        return Cell((float) std::get<int>(data));
    }
    if(is_float()){
        return *this;
    }
    
    throw std::runtime_error("Cannot convert value to float");
}

Cell Cell::to_string() const {
    if(is_null()){
        return Cell("NULL");
    }
    if(is_int()){
        return Cell(std::to_string(std::get<int>(data)));
    }
    if(is_float()){
        return Cell(std::to_string(std::get<float>(data)));
    }
    if(is_char()){
        return Cell(std::string(1, std::get<char>(data)));
    }
    if(is_string()){
        return *this;
    }
    
    throw std::runtime_error("Cannot convert value to string");
}