#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <shared_mutex>

#include "db/cell.h"

using TableRow = std::vector<Cell>;

class Table;

class TableHeader{
public:
    TableHeader(){}
    TableHeader(std::vector<std::string> column_names, std::vector<Cell::DataType> column_types);

    size_t column_count() const { return column_names.size(); }
    std::vector<Cell> parse_row(const std::vector<std::optional<std::string>>& data) const;
    
    static TableHeader join(const TableHeader& left, const TableHeader& right);
    
private: 
    std::vector<std::string> column_names;
    std::vector<Cell::DataType> column_types;
    
    friend void serialize_table(const Table& table, std::ostream& os);
};

class Table{
public:
    Table(TableHeader header);
    Table(const std::vector<std::string>& column_names, const std::vector<Cell::DataType>& column_types);
    
    Table(Table&& other) noexcept;

    void add_row(const std::vector<std::optional<std::string>>& row);

    Table filter(const std::string& predicate) const;
    static Table full_join(const Table& left, const Table& right);
private:
    TableHeader header;
    std::vector<TableRow> rows;

    void add_row(std::vector<Cell> data);
    
    mutable std::shared_mutex mutex;
    
    friend void serialize_table(const Table& table, std::ostream& os);
};

#endif
