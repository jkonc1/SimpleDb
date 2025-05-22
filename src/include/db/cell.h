#ifndef CELL_H
#define CELL_H

#include "db/exceptions.h"
#include <utility>
#include <variant>
#include <string>
#include <optional>

/**
 * @brief Represents a value in a database
 */
class Cell{
public:
    /**
     * @brief Data type of a cell
     */
    enum class DataType{
        Null,   
        String, 
        Int,   
        Float, 
        Char   
    };

    /**
     * @brief Construct Null cel
     */
    explicit Cell() : data(std::monostate()) {}
    
    /**
     * @brief Construct a cell of given value and type
        conversion is done if the types differ
     */
    template<class T>
    Cell(T&& value, DataType type) {
        Cell temp = construct_from(std::forward<T>(value));
        *this = temp.convert(type);
    }

    Cell(const Cell& other) : data(other.data){ }
    
    Cell& operator=(const Cell& other){
        data = other.data;
        return *this;
    }
    
    /**
     * @brief Get common promotion type for two data types
     */
    static DataType get_common_type(DataType left, DataType right);
    
    /**
     * @brief Promote two cells to a common type
     */
    static std::pair<Cell, Cell> promote_to_common(const Cell& left, const Cell& right);
    
    Cell operator+(const Cell& other) const;
    Cell operator-(const Cell& other) const;
    Cell operator*(const Cell& other) const;
    Cell operator/(const Cell& other) const;
    
    Cell operator+=(const Cell& other);
    Cell operator-=(const Cell& other);
    Cell operator*=(const Cell& other);
    Cell operator/=(const Cell& other);
    
    /**
     * @brief Equality operator - compares on converted values
            different types may equal
            Null is never equal to anything
     */
    bool operator==(const Cell& other) const;
    
    /**
     * @brief Inequality operator - compares on converted values
            different types may equal
            Null is never inequal to anything
     */
    bool operator!=(const Cell& other) const;
    
    /**
     * @brief Less than operator - compares on converted values
            Null is incomparable with anything
     */
    bool operator<(const Cell& other) const;
    
    /**
     * @brief Greater than operator - compares on converted values
            Null is incomparable with anything
     */
    bool operator>(const Cell& other) const;
    
    /**
     * @brief Less than or equal operator - compares on converted values
            Null is incomparable with anything
     */
    bool operator<=(const Cell& other) const;
    
    /**
     * @brief Greater than or equal operator - compares on converted values
            Null is incomparable with anything
     */
    bool operator>=(const Cell& other) const;
    
    
    /**
     * @brief Get the data type of the cell
     */
    DataType type() const;
    
    /**
     * @brief Get string representation of the cell value
     * @return String representation or `std::nullopt` for Null
     */
    std::optional<std::string> repr() const;
    
    /**
     * @brief Check if two cells are identical
            Checks on identity, different types are not identical
     */
    static bool is_identical(const Cell& left, const Cell& right);
    
private:
    /**
     * @brief Apply a binary operation on cells
       @throws InvalidQuery if the operation is not supported
     */
    template <class OP>
    Cell binary_op(const Cell& other) const {
        auto [left, right] = promote_to_common(*this, other);
        if(left.type() == DataType::Null){
            return Cell();
        }
        
        return std::visit([&](auto&& l, auto&& r){
            if constexpr(requires { OP{}(l, r); }) {
                return Cell(OP{}(l, r), left.type());
            } else {
                throw InvalidQuery("Invalid conversion");
                return Cell();
            }
        }, left.data, right.data);
    }
    
    /**
     * @brief Apply a predicate operation on cells
            Always returns false if one of the operands is Null
     */
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
    
    /**
     * @brief Construct a cell from a value
            The type of the value is maintained
     */
    template<class T>
    static Cell construct_from(T&& value) {
        Cell result;
        result.data = std::move(value);
        return result;
    }

    /**
     * @brief Convert cell to a different data type
       @return A the converted cell
     */
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
    /**
    size_t operator()(const Cell& cell){
        return std::hash<decltype(cell.data)>()(cell.data);
    }
};

#endif