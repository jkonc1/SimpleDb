#include "db/cell.h"
#include "db/exceptions.h"
#include "helper/overload.h"

#include <sstream>
#include <utility>

std::optional<std::string> Cell::repr() const {
    if(type() == DataType::Null){
        return std::nullopt;
    }
    
    return std::get<std::string>(convert(DataType::String).data);
}

std::pair<Cell, Cell> Cell::promote_to_common(const Cell& left, const Cell& right) {
    // the promotion order is int -> float -> string -> null
    // char -> string
    
    Cell::DataType type = DataType::Null;
    
    if(left.type() == right.type()){
        type = left.type();
    }
    else if(left.type() == DataType::Null || right.type() == DataType::Null){
        type = DataType::Null;
    }
    else if(left.type() == DataType::Int && right.type() == DataType::Float){
        type = DataType::Float;
    }
    else if(left.type() == DataType::Float && right.type() == DataType::Int){
        type = DataType::Float;
    }
    else{
        type = DataType::String;
    }
    
    return {left.convert(type), right.convert(type)};
}

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
    if (!(stream >> result) || stream.peek() != EOF) {
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

Cell Cell::operator+(const Cell& other) const {
    return binary_op<std::plus<>>(other);
}

Cell Cell::operator-(const Cell& other) const {
    return binary_op<std::minus<>>(other);
}

Cell Cell::operator*(const Cell& other) const {
    return binary_op<std::multiplies<>>(other);
}

Cell Cell::operator/(const Cell& other) const {
    return binary_op<std::divides<>>(other);
}

Cell Cell::operator+=(const Cell& other) {
    *this = *this + other;
    return *this;
}

Cell Cell::operator-=(const Cell& other) {
    *this = *this - other;
    return *this;
}

Cell Cell::operator*=(const Cell& other) {
    *this = *this * other;
    return *this;
}

Cell Cell::operator/=(const Cell& other) {
    *this = *this / other;
    return *this;
}

bool Cell::operator==(const Cell& other) const{
    return predicate<std::equal_to<>>(other);
}

bool Cell::operator!=(const Cell& other) const{
    return predicate<std::not_equal_to<>>(other);
}

bool Cell::operator<(const Cell& other) const{
    return predicate<std::less<>>(other);
}

bool Cell::operator>(const Cell& other) const{
    return predicate<std::greater<>>(other);
}

bool Cell::operator<=(const Cell& other) const{
    return predicate<std::less_equal<>>(other);
}

bool Cell::operator>=(const Cell& other) const{
    return predicate<std::greater_equal<>>(other);
}
