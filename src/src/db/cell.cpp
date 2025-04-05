#include "db/cell.h"
#include "db/exceptions.h"

#include <cmath>
#include <sstream>
#include <utility>

std::optional<std::string> Cell::repr() const {
    if(type() == DataType::Null){
        return std::nullopt;
    }
    
    return std::get<std::string>(convert(DataType::String).data);
}

std::pair<Cell, Cell> Cell::promote_to_common(const Cell& a, const Cell& b) {
    // the promotion order is int -> float -> string -> null
    // char -> string
    
    Cell::DataType type;
    
    if(a.type() == b.type()){
        type = a.type();
    }
    else if(a.type() == DataType::Null || b.type() == DataType::Null){
        type = DataType::Null;
    }
    else if(a.type() == DataType::Int && b.type() == DataType::Float){
        type = DataType::Float;
    }
    else if(a.type() == DataType::Float && b.type() == DataType::Int){
        type = DataType::Float;
    }
    else{
        type = DataType::String;
    }
    
    return {a.convert(type), b.convert(type)};
}

template<class... Ts>
struct overload : Ts... { using Ts::operator()...; };

Cell::DataType Cell::type() const {
    return std::visit(
        overload{
            [](const int&){ return DataType::Int; },
            [](const float&){ return DataType::Float; },
            [](const std::string&){ return DataType::String; },
            [](const char&){ return DataType::Char; },
            [](const std::monostate&){ return DataType::Null; }
        },
        data
    );
}

Cell Cell::convert(DataType target_type) const {
    switch(target_type){
        case DataType::Null:
            return Cell();
        case DataType::String:
            return convert_to_string();
        case DataType::Float:
            return convert_to_float();
        case DataType::Int:
            return convert_to_int();
        case DataType::Char:
            return convert_to_char();
    }
    std::unreachable();
}

template <class T>
T string_to(const std::string& value) {
    std::stringstream stream(value);
    T result;
    stream >> result;
    if (!(stream >> result) || !(stream.eof())) {
        throw InvalidConversion("Could not convert string to value");
    }
    return result;
}

Cell Cell::convert_to_int() const {
    return std::visit(
        overload{
            [](const int& value){ return Cell(value); },
            [](const std::string& value){ return Cell(string_to<int>(value)); },
            [](const std::monostate&){ return Cell(); },
            [](auto&&) -> Cell { throw InvalidConversion("Can't value to int"); },
        },
        data
    );
}

Cell Cell::convert_to_float() const {
    return std::visit(
        overload{
            [](const int& value){ return Cell(static_cast<float>(value)); },
            [](const float& value){ return Cell(value); },
            [](const std::string& value){ return Cell(string_to<float>(value)); },
            [](const std::monostate&){ return Cell(); },
            [](const auto&&) -> Cell { throw InvalidConversion("Can't convert value to float"); },
        },
        data
    );
}

Cell Cell::convert_to_char() const {
    return std::visit(
        overload{
            [](const char& value){ return Cell(value); },
            [](const std::monostate&){ return Cell(); },
            [](const std::string& value){ return Cell(string_to<char>(value)); },
            [](const auto&&) -> Cell { throw InvalidConversion("Can't convert value to char"); },
        },
        data
    );
}

Cell Cell::convert_to_string() const {
    return std::visit(
        overload{
            [](const int& value){ return Cell(std::to_string(value)); },
            [](const float& value){ return Cell(std::to_string(value)); },
            [](const std::string& value){ return Cell(value); },
            [](const char& value){ return Cell(std::string(1, value)); },
            [](const std::monostate&){ return Cell(); }
        },
        data
    );
}