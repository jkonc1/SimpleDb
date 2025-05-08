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

Cell::DataType Cell::get_common_type(DataType left, DataType right){
    // the promotion order is int -> float -> string -> null
    // char -> string
    
    if(left == DataType::Null || right == DataType::Null){
        return DataType::Null;
    }
    if(left == DataType::Int && right == DataType::Float){
        return DataType::Float;
    }
    if(left == DataType::Float && right == DataType::Int){
        return DataType::Float;
    }
    if(left == right && left != DataType::Char){
        // char must always be promoted
        return left;
    }
    return DataType::String;
}

std::pair<Cell, Cell> Cell::promote_to_common(const Cell& left, const Cell& right) {
    Cell::DataType type = get_common_type(left.type(), right.type());
    
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

template<class T>
std::string my_to_string(const T& value){
    return std::format("{}", value);
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
            [](const int& value){ return Cell(my_to_string(value)); },
            [](const float& value){ return Cell(my_to_string(value)); },
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

bool Cell::is_identical(const Cell& left, const Cell& right){
    return left.data == right.data;
}
