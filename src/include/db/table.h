#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <shared_mutex>
#include <map>
#include <memory>
#include <valarray>
#include <functional>

#include "db/cell.h"
#include "parse/token_stream.h"
#include "db/expression.h"

using CellVector = std::valarray<Cell>;
using BoolVector = std::valarray<bool>;

using TableRow = std::vector<Cell>;

class Table;

struct ColumnDescriptor{
    std::string alias;
    std::string name;
    Cell::DataType type;
    size_t index;
};

class TableHeader{
public:
    TableHeader(){}
    TableHeader(std::vector<std::string> column_names, std::vector<Cell::DataType> column_types);
    
    const std::vector<ColumnDescriptor>& get_columns() const;
    
    size_t column_count() const { return columns.size(); }
    
    std::optional<ColumnDescriptor> get_column_info(const std::string& name) const;
    
    std::vector<Cell> create_row(const std::map<std::string, std::string>& data) const;
    
    TableHeader add_alias(const std::string& alias) const;
    
    static TableHeader join(const TableHeader& left, const TableHeader& right);
    
private: 
    std::vector<ColumnDescriptor> columns;
    
    TableHeader(const TableHeader& left, const TableHeader& right);
    
    std::map<std::string, size_t> column_to_index;
};

class VariableList;

class Table{
public:
    Table(TableHeader header);
    Table(const std::vector<std::string>& column_names, const std::vector<Cell::DataType>& column_types);
    
    Table(Table&& other) noexcept;
    
    void filter_by_condition(TokenStream& stream, const VariableList& variables,
        std::function<Table(TokenStream&, const VariableList&)> select_callback);

    void add_row(const std::map<std::string, std::string>& values);
    
    std::vector<Table> group_by(const std::vector<std::string>& group_columns);
    
    const std::vector<ColumnDescriptor>& get_columns() const;

    static Table cross_product(std::vector<std::pair<const Table&, std::string>> tables);
private:
    TableHeader header;
    std::vector<TableRow> rows;

    void add_row(std::vector<Cell> data);
    
    size_t row_count() const {
        return rows.size();
    }
    
    std::valarray<Cell> evaluate_expression(TokenStream& stream, const VariableList& variables) const;
    
    std::unique_ptr<ExpressionNode> parse_additive_expression(TokenStream& stream, const VariableList& variables) const;
    std::unique_ptr<ExpressionNode> parse_multiplicative_expression(TokenStream& stream, const VariableList& variables) const;
    std::unique_ptr<ExpressionNode> parse_primary_expression(TokenStream& stream, const VariableList& variables) const;
    
    mutable std::shared_mutex mutex;
    
    friend void serialize_table(const Table& table, std::ostream& os);
    
    friend class ConditionEvaluation;
};

#endif
