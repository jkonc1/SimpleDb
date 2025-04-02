#include "db/table.h"

TableHeader::TableHeader(std::vector<std::string> column_names, std::vector<Cell::DataType> column_types):
    column_names(std::move(column_names)), column_types(std::move(column_types))
{
    if(column_names.size() != column_types.size()){
        throw std::runtime_error("Column count mismatch");
    }
}

TableHeader TableHeader::join(const TableHeader& left, const TableHeader& right){
    std::vector<std::string> names = left.column_names;
    names.insert(names.end(), right.column_names.begin(), right.column_names.end());
    
    std::vector<Cell::DataType> types = left.column_types;
    types.insert(types.end(), right.column_types.begin(), right.column_types.end());
    
    return TableHeader(names, types);
}

bool TableHeader::validate_row(const TableRow& row) const{
    if(row.size() != column_names.size()){
        return false;
    }
    
    for(size_t column = 0; column < row.size(); ++column){
        Cell::DataType type = column_types[column];
        if(row[column].type() != type && row[column].type() != Cell::DataType::Null){
            return false;
        }
    }
    
    return true;
}

TableRow join_rows(const TableRow& left, const TableRow& right){
    std::vector<Cell> result_cells = left;
    result_cells.insert(result_cells.end(), right.begin(), right.end());
    return result_cells;
}

Table::Table(TableHeader header):
    header(std::move(header))
{
}

Table::Table(const std::vector<std::string>& column_names, const std::vector<Cell::DataType>& column_types):
    Table(TableHeader(column_names, column_types))
{
}

Table::Table(Table&& other) noexcept
{
    auto lock = std::lock_guard(other.mutex);
    header = std::move(other.header);
    rows = std::move(other.rows);
}

void Table::add_row(std::vector<Cell> data){
    auto lock = std::lock_guard(mutex);
    
    if(!header.validate_row(data)){
        throw std::runtime_error("Row mismatches table schema");
    }
    
    rows.push_back(std::move(data));
}

Table Table::full_join(const Table& left, const Table& right){
    Table result(TableHeader::join(left.header, right.header));
    
    std::vector<TableRow> rows;
    for(auto& left_row : left.rows){
        for(auto& right_row : right.rows){
            rows.push_back(join_rows(left_row, right_row));
        }
    }
    
    return result;
}