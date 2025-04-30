#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <shared_mutex>
#include <map>
#include <unordered_map>

#include "db/cell.h"

using TableRow = std::vector<Cell>;

class Table;

struct ColumnDescriptor{
    std::string name;
    Cell::DataType type;
};

class TableHeader{
public:
    TableHeader(){}
    TableHeader(std::vector<std::string> column_names, std::vector<Cell::DataType> column_types);
    
    const std::vector<ColumnDescriptor>& get_columns() const;
    
    size_t column_count() const { return columns.size(); }
    std::vector<Cell> create_row(const std::map<std::string, std::string>& data) const;
    
    static TableHeader join(const TableHeader& left, const TableHeader& right);
    
private: 
    std::vector<ColumnDescriptor> columns;
    
    TableHeader(const TableHeader& left, const TableHeader& right);
    
    std::map<std::string, size_t> column_to_index;
};

class Table{
public:
    Table(TableHeader header);
    Table(const std::vector<std::string>& column_names, const std::vector<Cell::DataType>& column_types);
    
    Table(Table&& other) noexcept;

    void add_row(const std::map<std::string, std::string>& values);
    
    const std::vector<ColumnDescriptor>& get_columns() const;

    std::unordered_map<Cell, Table> split_by_expression(const std::string& condition) const;
    
    static Table full_join(const Table& left, const Table& right);
protected:
    TableHeader header;
    std::vector<TableRow> rows;

    void add_row(std::vector<Cell> data);
    
    mutable std::shared_mutex mutex;
    
    friend void serialize_table(const Table& table, std::ostream& os);
};

#endif
