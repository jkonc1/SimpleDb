#include "db/table_serialization.h"
#include "csv/csv.h"
#include "db/exceptions.h"

#include <ranges>

namespace {
std::string type_to_string(Cell::DataType type){
    switch(type){
        case Cell::DataType::Null:
            return "NULL";
        case Cell::DataType::Int:
            return "INT";
        case Cell::DataType::Float:
            return "FLOAT";
        case Cell::DataType::String:
            return "STRING";
        case Cell::DataType::Char:
            return "CHAR";
        default:
            throw std::runtime_error("Invalid data type");
    }
}

Cell::DataType string_to_type(std::string type){
    for(auto& c : type){
        c = std::toupper(c);
    }
    
    if(type == "NULL"){
        return Cell::DataType::Null;
    }else if(type == "INT"){
        return Cell::DataType::Int;
    }else if(type == "FLOAT"){
        return Cell::DataType::Float;
    }else if(type == "STRING"){
        return Cell::DataType::String;
    }else if(type == "CHAR"){
        return Cell::DataType::Char;
    }else{
        throw std::runtime_error("Invalid data type");
    }
}
}

Table load_table(std::istream& is){
    auto data = read_csv(is);
    
    if(data.size() < 2){
        throw ParsingError("Invalid table data");
    }
    
    auto column_names_opt = data[0];
    size_t column_count = column_names_opt.size();
    
    std::vector<std::string> column_names;
    
    for(auto&& name : column_names_opt){
        if(!name.has_value()){
            throw ParsingError("Invalid (null) column name");
        }
        column_names.push_back(name.value());
    }
    
    auto column_types_opt = data[1];
    
    std::vector<Cell::DataType> column_types;
    
    if(column_types_opt.size() != column_count){
        throw ParsingError("Invalid column type count");
    }
    
    for(auto&& type : column_types_opt){
        if(!type.has_value()){
            throw ParsingError("Invalid (null) column type");
        }
        column_types.push_back(string_to_type(type.value()));
    }
    
    Table result(column_names, column_types);
    
    for(auto&& row : data | std::views::drop(2)){
        std::map<std::string, std::string> assignments;
        
        for(auto&& [index, value] : std::views::enumerate(row)){
            if(!value.has_value()) {
                continue;
            }
            assignments.emplace(column_names[index], value.value());
        }
        
        result.add_row(assignments);
    }
    
    return result;
}

VoidableRow dump_row(const TableRow& row){
    VoidableRow result;
    
    for(auto&& cell : row){
        result.push_back(cell.repr());
    }
    
    return result;
}

void serialize_table(const Table& table, std::ostream& os) {
    // TODO this probably shouldn't be a friend in the long run
    VoidableTable result;
    
    VoidableRow names_row;
    
    for(const auto& column : table.get_columns()){
        names_row.push_back(column.name);
    }
    
    VoidableRow types_row;
    
    for(auto&& column : table.get_columns()){
        types_row.push_back(type_to_string(column.type));
    }
    
    result.push_back(names_row);
    result.push_back(types_row);
    
    for(auto&& row : table.rows){
        result.push_back(dump_row(row));
    }
    
    write_csv(os, result);
}