#include "db/variable_list.h"
#include "db/exceptions.h"

BoundRow::BoundRow(const TableHeader& header, const TableRow& row):
    header(header), row(row) {}

// apparently references can't be in std::optional...
std::optional<std::pair<const Cell*, Cell::DataType>> BoundRow::get_value(const std::string& name) const{
    auto column_info = header.get_column_info(name);
    
    if(!column_info.has_value()){
        return std::nullopt;
    }
    
    auto [_alias, _name, type, index] = column_info.value();
    
    return std::make_pair(&row[index], type);
}


std::pair<const Cell*, Cell::DataType> VariableList::get_info(const std::string& name) const{
    std::optional<std::pair<const Cell*, Cell::DataType>> result;
    
    for(const auto& member : members){
        auto value = member.get_value(name);
        
        if(!value.has_value()) { 
            continue;
        }
        
        if(result.has_value()){
            throw InvalidQuery("Non-unique variable name: " + name);
        }
        
        result = value;
    }
    
    if(!result.has_value()){
        throw InvalidQuery("Variable not found: " + name);
    }
    
    return result.value();
}

const Cell& VariableList::get_value(const std::string& name) const{
    auto [value, type] = get_info(name);
    return *value;
}

Cell::DataType VariableList::get_type(const std::string& name) const{
    auto [_, type] = get_info(name);
    return type;
}


VariableList VariableList::operator+(BoundRow other) const{
    VariableList result(*this);
    result.members.push_back(other);
    return result;
}
