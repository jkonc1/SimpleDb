#include "db/table.h"
#include "db/exceptions.h"

#include <ranges>

TableHeader::TableHeader(std::vector<std::string> column_names, std::vector<Cell::DataType> column_types)
{
    if(column_names.size() != column_types.size()){
        throw std::runtime_error("Column count mismatch");
    }
    
    for(auto&& [column_name, column_type] : std::views::zip(column_names, column_types)){
        ColumnDescriptor column(column_name, column_type);
        
        columns.push_back(column);
        
        column_to_index[column_name] = columns.size() - 1;
    }
}

TableHeader TableHeader::join(const TableHeader& left, const TableHeader& right){
    return TableHeader(left, right);
}

TableHeader::TableHeader(const TableHeader& left, const TableHeader& right) :
    columns(left.columns), column_to_index(left.column_to_index){
    
    for(auto&& [column_name, column_type] : right.columns){
        ColumnDescriptor column(column_name, column_type);
        
        columns.push_back(column);
        
        column_to_index[column_name] = columns.size() - 1;
    }
}

TableRow TableHeader::create_row(const std::map<std::string, std::string>& data) const {
    std::vector<Cell> cells(column_count());
    
    for(auto&& [column_name, value] : data){
        if(!column_to_index.contains(column_name)){
            throw InvalidQuery("Column '" + column_name + "' does not exist");
        }
        size_t index = column_to_index.at(column_name);
        
        cells[index] = Cell(value, columns[index].type);
    }
    
    return cells;
}

TableRow join_rows(const TableRow& left, const TableRow& right){
    // TODO: this is a bit weird with remade indexing, probably move to header
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

Table::Table(Table&& other) noexcept : 
    header(std::move(other.header)), rows(std::move(other.rows))
{
    auto lock = std::lock_guard(other.mutex);
}

void Table::add_row(std::vector<Cell> data){
    auto lock = std::lock_guard(mutex);
    
    rows.push_back(std::move(data));
}

void Table::add_row(const std::map<std::string, std::string>& values){
    add_row(header.create_row(values));
}

Table Table::full_join(const Table& left, const Table& right){
    Table result(TableHeader::join(left.header, right.header));
    
    std::vector<TableRow> rows;
    for(const auto& left_row : left.rows){
        for(const auto& right_row : right.rows){
            rows.push_back(join_rows(left_row, right_row));
        }
    }
    
    return result;
}

const std::vector<ColumnDescriptor>& Table::get_columns() const{
    return header.get_columns();
}

const std::vector<ColumnDescriptor>& TableHeader::get_columns() const{
    return columns;
}
