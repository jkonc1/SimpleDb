#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <shared_mutex>
#include <map>
#include <memory>

#include "db/cell.h"
#include "parse/token_stream.h"
#include "db/expression.h"

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
    size_t get_column_index(const std::string& name) const;
    std::vector<Cell> create_row(const std::map<std::string, std::string>& data) const;
    
    static TableHeader join(const TableHeader& left, const TableHeader& right);
    
private: 
    std::vector<ColumnDescriptor> columns;
    
    TableHeader(const TableHeader& left, const TableHeader& right);
    
    std::map<std::string, size_t> column_to_index;
};

class RowReference{
public:
    RowReference(const TableHeader& header, const TableRow& row);
    
    const Cell& operator[](const std::string& name) const;
        
private:
    const TableHeader& header;
    const TableRow& row;
};

class Table{
public:
    Table(TableHeader header);
    Table(const std::vector<std::string>& column_names, const std::vector<Cell::DataType>& column_types);
    
    Table(Table&& other) noexcept;

    void add_row(const std::map<std::string, std::string>& values);
    
    const std::vector<ColumnDescriptor>& get_columns() const;

    static Table full_join(const Table& left, const Table& right);
private:
    TableHeader header;
    std::vector<TableRow> rows;

    void add_row(std::vector<Cell> data);
    
    size_t row_count() const {
        return rows.size();
    }
    
    std::vector<Cell> evaluate_expression(TokenStream& stream) const;
    
    std::unique_ptr<ExpressionNode> parse_additive_expression(TokenStream& stream) const;
    std::unique_ptr<ExpressionNode> parse_multiplicative_expression(TokenStream& stream) const;
    std::unique_ptr<ExpressionNode> parse_primary_expression(TokenStream& stream) const;
    
    std::vector<bool> evaluate_condition(TokenStream& stream) const;
    std::vector<bool> evaluate_conjunctive_condition(TokenStream& stream) const;
    std::vector<bool> evaluate_disjunctive_condition(TokenStream& stream) const;
    std::vector<bool> evaluate_primary_condition(TokenStream& stream) const;
        
    mutable std::shared_mutex mutex;
    
    friend void serialize_table(const Table& table, std::ostream& os);
};

#endif
