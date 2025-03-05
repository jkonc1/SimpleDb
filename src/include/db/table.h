#ifndef TABLE_H
#define TABLE_H

#include <string>

#include "db/cell.h"

struct TableHeader{
    std::vector<std::string> column_names;
    std::vector<Cell::DataType> column_types;
};

class TableRow{
public: 
    TableRow(std::shared_ptr<TableHeader> header, std::vector<Cell> data);
    
    static std::shared_ptr<TableRow> join(const std::shared_ptr<TableRow>& left, const std::shared_ptr<TableRow>& right);
private:
    std::shared_ptr<TableHeader> header;
    std::vector<Cell> cells;
};

class Table{
public:
    Table(const std::vector<std::string>& column_names, const std::vector<Cell::DataType>& column_types);
    
    void add_row(const std::vector<Cell>& data);
    
    void dump(std::ostream& os);
    static std::shared_ptr<Table> load(std::istream& is);
    
    std::shared_ptr<Table> filter(const std::string& predicate);
    static std::shared_ptr<Table> full_join(const shared_ptr<Table>& left, const std::shared_ptr<Table>& right);
private:
    TableHeader header;
    std::vector<TableRow> rows;
    
    std::shared_mutex mutex;
};

#endif