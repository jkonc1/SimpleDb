#ifndef VARIABLE_LIST_H
#define VARIABLE_LIST_H

#include "db/table.h"

struct BoundRow {
    BoundRow(const TableHeader& header, const TableRow& row);
    
    std::optional<std::pair<const Cell*, Cell::DataType>> get_value(const std::string& name) const;
    
private:
    const TableHeader& header;
    const TableRow& row;
};

class VariableList{
public:
    VariableList(){};
    
    const Cell& get_value(const std::string& name) const;
    Cell::DataType get_type(const std::string& name) const;
    
    VariableList operator+(BoundRow other) const;
        
private:
    std::vector<BoundRow> members;
    
    std::pair<const Cell*, Cell::DataType> get_info(const std::string& name) const;
};

#endif