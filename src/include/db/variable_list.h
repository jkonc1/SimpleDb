#ifndef VARIABLE_LIST_H
#define VARIABLE_LIST_H

#include "db/table.h"

/**
 * @brief A row combined with the header for variable access
 */
struct BoundRow {
    BoundRow(const TableHeader& header, const TableRow& row);
    
    /**
     * @brief Get a value by column name
     * @param name The name of the column
     * @return The value and column type (`std::nullopt` if column not found)
     */
    std::optional<std::pair<const Cell*, Cell::DataType>> get_value(const std::string& name) const;
    
private:
    const TableHeader& header;
    const TableRow& row;
};

/**
 * @brief Provide access to several rows combined horizontally by name
 */
class VariableList{
public:
    VariableList(){};
    
    /**
     * @brief Get a variable value by name
     * @throws InvalidQuery if the variable is not found or ambiguous
     */
    const Cell& get_value(const std::string& name) const;
    
    /**
     * @brief Get a variable type by name
     * @throws InvalidQuery if the variable is not found or ambiguous
     */
    Cell::DataType get_type(const std::string& name) const;
    
    /**
     * @brief Add a row to VariableList
     */
    VariableList operator+(BoundRow other) const;
        
private:
    std::vector<BoundRow> members;
    
    /**
     * @brief Look up a variable
     */
    std::pair<const Cell*, Cell::DataType> get_info(const std::string& name) const;
};

#endif