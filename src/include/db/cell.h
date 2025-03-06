#ifndef CELL_H
#define CELL_H

#include <variant>
#include <string>

class Cell{
public:
    Cell(const std::string& str) : data(str) {}
    Cell(const int num) : data(num) {}
    Cell(const float num) : data(num) {}
    Cell(const char ch) : data(ch) {}
    Cell() : data(std::monostate()) {}
    
    enum class DataType{
        Null,
        String,
        Int,
        Float,
        Char
    };
    
    bool is_null() const {
        return std::holds_alternative<std::monostate>(data);
    }
    
    bool is_int() const {
        return std::holds_alternative<int>(data);
    }
    
    bool is_float() const {
        return std::holds_alternative<float>(data);
    }
    
    bool is_char() const {
        return std::holds_alternative<char>(data);
    }
    
    bool is_string() const {
        return std::holds_alternative<std::string>(data);
    }
    
    static std::pair<Cell, Cell> promote_to_common(const Cell& a, const Cell& b);
    
    Cell operator+(const Cell& other) const;
    Cell operator-(const Cell& other) const;
    Cell operator*(const Cell& other) const;
    Cell operator/(const Cell& other) const;
    
private:
    bool is_number() const {
        return is_int() || is_float();
    }
    
    Cell to_float() const;
    Cell to_string() const;
    
    std::variant<std::monostate, int, float, char, std::string> data;
};

#endif