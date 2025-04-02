#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <memory>
#include <shared_mutex>
#include <optional>

#include "db/cell.h"

using TableRow = std::vector<Cell>;

struct TableHeader{
    std::vector<std::string> column_names;
    std::vector<Cell::DataType> column_types;

    size_t column_count() const { return column_names.size(); }

    TableHeader(){}
    TableHeader(std::vector<std::string> column_names, std::vector<Cell::DataType> column_types);

    static TableHeader join(const TableHeader& left, const TableHeader& right);

    bool validate_row(const TableRow& row) const;
};

class Table{
public:
    Table(TableHeader header);
    Table(const std::vector<std::string>& column_names, const std::vector<Cell::DataType>& column_types);
    
    Table(Table&& other) noexcept;

    void add_row(std::vector<Cell> data);

    Table filter(const std::string& predicate) const;
    static Table full_join(const Table& left, const Table& right);
private:
    TableHeader header;
    std::vector<TableRow> rows;

    std::shared_mutex mutex;
    
    friend void serialize_table(const Table& table, std::ostream& os);
};

#endif
