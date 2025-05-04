#ifndef CELL_H
#define CELL_H

#include <utility>
#include <variant>
#include <string>
#include <optional>

class Cell{
public:
    enum class DataType{
        Null,
        String,
        Int,
        Float,
        Char
    };

    explicit Cell() : data(std::monostate()) {}
    
    template<class T>
    Cell(T&& value, DataType type) {
        Cell temp(std::forward<T>(value));
        *this = temp.convert(type);
    }
    
    static std::pair<Cell, Cell> promote_to_common(const Cell& left, const Cell& right);
    
    Cell operator+(const Cell& other) const;
    Cell operator-(const Cell& other) const;
    Cell operator*(const Cell& other) const;
    Cell operator/(const Cell& other) const;
    
    Cell operator+=(const Cell& other);
    Cell operator-=(const Cell& other);
    Cell operator*=(const Cell& other);
    Cell operator/=(const Cell& other);
    
    bool operator==(const Cell& other) const;
    bool operator!=(const Cell& other) const;
    bool operator<(const Cell& other) const;
    bool operator>(const Cell& other) const;
    bool operator<=(const Cell& other) const;
    bool operator>=(const Cell& other) const;
    
    
    DataType type() const;
    
    std::optional<std::string> repr() const;
    
private:

    template <class OP>
    Cell binary_op(const Cell& other) const {
        auto [left, right] = promote_to_common(*this, other);
        
        return std::visit([&](auto&& l, auto&& r){
            if constexpr(requires { OP{}(l, r); }) {
                return Cell(OP{}(l, r), left.type());
            } else {
                return Cell();
            }
        }, left.data, right.data);
    }
    
    template<class OP>
    bool predicate(const Cell& other) const {
        auto [left, right] = promote_to_common(*this, other);
        
        return std::visit([&](auto&& l, auto&& r){
            using L = std::decay_t<decltype(l)>;
            using R = std::decay_t<decltype(r)>;
            if constexpr(std::is_same_v<L, std::monostate>) {
                return false;
            } else if constexpr(std::is_same_v<L, R>) {
                return OP{}(l, r);
            }
            std::unreachable();
            return false;
        }, left.data, right.data);
    }
    
    template<class T>
    explicit Cell(T&& value) : data(std::forward<T>(value)) {}

    Cell convert(DataType target_type) const;
    
    Cell convert_to_string() const;
    Cell convert_to_int() const;
    Cell convert_to_float() const;
    Cell convert_to_char() const;
    
    std::variant<std::monostate, int, float, char, std::string> data;
    
    friend std::hash<Cell>;
};

template<>
struct std::hash<Cell>{
    size_t operator()(const Cell& cell){
        return std::hash<decltype(cell.data)>()(cell.data);
    }
};

#endif