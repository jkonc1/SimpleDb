#include "db/table.h"
#include "csv/csv.h"

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
    if(row.cells.size() != column_names.size()){
        return false;
    }
    
    for(size_t column = 0; column < row.cells.size(); ++column){
        Cell::DataType type = column_types[column];
        if(row[column].fits_in_type(type)){
            return false;
        }
    }
    
    return true;
}

TableRow join_rows(const TableRow& left, const TableRow& right){
    std::vector<Cell> result_cells = left.cells;
    result_cells.insert(result_cells.end(), right.cells.begin(), right.cells.end());
    return TableRow(std::move(result_cells));
}

Table::Table(const std::vector<std::string>& column_names, const std::vector<Cell::DataType>& column_types){
    header.column_names = column_names;
    header.column_types = column_types;
}

void Table::add_row(std::vector<Cell> data){
    auto lock = std::lock_guard(mutex);
    
    if(!header.validate_row(data)){
        throw std::runtime_error("Row mismatches table schema");
    }
    
    rows.push_back(std::move(data));
}

void Table::add_row(std::vector<std::optional<std::string>> data){
    std::vector<Cell> cells;
    
    for(size_t column = 0; column < data.size(); ++column){
        Cell::DataType type = header.column_types[column];
        
        if(data[column].has_value()){
            cells.push_back(Cell(type, data[column].value()));
        } else {
            cells.push_back(Cell(type));
        }
    }
    
    add_row(std::move(cells));
}

Table Table::full_join(const Table& left, const Table& right){
    TableHeader header = TableHeader::join(left.header, right.header);
    
    std::vector<TableRow> rows;
    for(auto&& left_row : left.rows){
        for(auto&& right_row : right.rows){
            rows.push_back(join_rows(left_row, right_row));
        }
    }
    
    return Table(std::move(header), std::move(rows));
}

Table::Table(TableHeader&& header, std::vector<TableRow>&& rows):
    header(std::move(header)),
    rows(std::move(rows))
{}




// TODO extract everything below

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
            return "Char";
        default:
            throw std::runtime_error("Invalid data type");
    }
}

Cell::CellType string_to_type(const std::string& type){
    if(type == "NULL"){
        return Cell::DataType::Null;
    }else if(type == "INT"){
        return Cell::DataType::Int;
    }else if(type == "FLOAT"){
        return Cell::DataType::Float;
    }else if(type == "STRING"){
        return Cell::DataType::String;
    }else if(type == "Char"){
        return Cell::DataType::Char;
    }else{
        throw std::runtime_error("Invalid data type");
    }
}

Table::Table(std::istream& is){
    // TODO extract somewhere in a separate file
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
        column_names.push_back(name);
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
        column_types.push_back(string_to_type(type));
    }
    
    header = TableHeader(std::move(column_names), std::move(column_types));
    
    for(size_t row = 2; row < data.size(); row++){
        add_row(data[row]);
    }
}

VoidableRow dump_row(const TableRow& row){
    VoidableRow result;
    
    for(auto&& cell : row){
        cells.push_back(cell.dump());
    }
    
    return cells;
}

void Table::dump(std::ostream& os) const{
    // TODO also extract elsewhere
    
    VoidableTable table;
    
    VoidableRow names_row;
    
    for(auto&& name : header.column_names()){
        names_row.push_back(name);
    }
    
    VoidableRow types_row;
    
    for(auto&& type : header.column_types()){
        types_row.push_back(type_to_string(type));
    }
    
    table.push_back(names_row);
    
    for(auto&& row : rows){
        table.push_back(dump_row(row));
    }
    
    write_csv(table, os);
}
