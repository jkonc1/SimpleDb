#include "db/table.h"
#include "db/condition_evaluation.h"
#include "db/exceptions.h"
#include "db/expression.h"
#include "db/variable_list.h"
#include "parse/token_to_cell.h"
#include "helper/read_array.h"
#include "helper/like.h"

#include <cassert>
#include <mutex>
#include <ranges>
#include <set>
#include <algorithm>
#include <functional>

TableHeader::TableHeader(std::vector<std::string> column_names, std::vector<Cell::DataType> column_types)
{
    if(column_names.size() != column_types.size()){
        throw std::runtime_error("Column count mismatch");
    }
    
    for(auto&& [column_name, column_type, index] : std::views::zip(column_names, column_types, std::views::iota((size_t)0))){
        ColumnDescriptor column = {.alias = "", .name = column_name, .type = column_type, .index=index};
        
        columns.push_back(column);
        
        column_to_index[column_name] = columns.size() - 1;
    }
}

TableHeader TableHeader::join(const TableHeader& left, const TableHeader& right){
    return TableHeader(left, right);
}

TableHeader TableHeader::add_alias(const std::string& alias) const {
    TableHeader header(*this);
    
    for(auto& column : header.columns){
        column.alias = alias;
    }
    
    return header;
}

std::optional<ColumnDescriptor> TableHeader::get_column_info(const std::string& name) const {
    if(!column_to_index.contains(name)){
        return std::nullopt;
    }
    
    size_t index = column_to_index.at(name);
    
    return columns[index];
}

TableHeader::TableHeader(const TableHeader& left, const TableHeader& right) :
    columns(left.columns), column_to_index(left.column_to_index){
    
    for(auto&& column : right.columns){
        ColumnDescriptor column_copy(column);
        
        size_t index = columns.size();
        
        column_copy.index = index;
        
        columns.push_back(column_copy);
        
        column_to_index[column_copy.name] = index;
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

Table Table::cross_product(std::vector<std::pair<const Table&, std::string>> tables){
    assert(!tables.empty());
    
    TableHeader header = tables[0].first.header.add_alias(tables[0].second);
    std::vector<TableRow> rows = tables[0].first.rows;
    
    for(auto& [table, alias] : tables | std::views::drop(1)){
        std::vector<TableRow> new_rows;
        for(const auto& row1 : rows){
            for(const auto& row2 : table.rows){
                std::vector<Cell> combined = row1;
                combined.append_range(row2);
                new_rows.push_back(combined);
            }
        }
        header = TableHeader::join(header, table.header.add_alias(alias));
        rows = std::move(new_rows);
    }
    
    Table result(std::move(header));
    
    for(const auto& row : rows){
        result.add_row(row);
    }
    
    return result;
}

const std::vector<ColumnDescriptor>& Table::get_columns() const{
    return header.get_columns();
}

const std::vector<ColumnDescriptor>& TableHeader::get_columns() const{
    return columns;
}

static CellVector get_distinct(const CellVector& values){
    std::set<Cell> values_set(std::begin(values), std::end(values));
    
    CellVector result(values_set.size());
    
    std::copy(std::begin(values_set), std::end(values_set), std::begin(result));
    
    return result;
}

std::unique_ptr<ExpressionNode> Table::parse_primary_expression(TokenStream& stream, const VariableList& variables) const {
    auto next_token = stream.get_token();
    
    if(next_token.type != TokenType::Identifier){
        
        Cell value = parse_token_to_cell(next_token);
        
        return std::make_unique<ConstantNode>(value);
    }
    
    if(next_token.like("NULL")){
        return std::make_unique<ConstantNode>(Cell());
    }
    
    if(next_token.like("COUNT")){
        
    }
    
    if(next_token.like("MAX") || next_token.like("MIN") || next_token.like("SUM") || next_token.like("AVG")){
        // TODO extract
         
        stream.ignore_token("(");
        
        bool is_distinct = stream.try_ignore_token("DISTINCT");
        
        auto values = evaluate_expression(stream, variables);
        
        stream.ignore_token(")");
        
        if(is_distinct){
            if(next_token.like("MAX") || next_token.like("MIN")){
                throw InvalidQuery("DISTINCT is not supported for MAX and MIN");
            }
            values = get_distinct(values);
        }
        
        Cell value;
        
        if(next_token.like("MAX")){
            value = values.max();
        } else if(next_token.like("MIN")){
            value = values.min();
        }
        else{
            // sum type
            
            value = values[0];
            
            for(size_t i = 1; i < values.size(); ++i){
                value += values[i];
            }
            
            if(next_token.like("AVG")){
                value /= Cell((int)values.size(), Cell::DataType::Int);
            }
        }
        
        return std::make_unique<ConstantNode>(value);
    }
    
    // othervise it's a column name
    
    return std::make_unique<VariableNode>(next_token.value);
}

std::unique_ptr<ExpressionNode> Table::parse_multiplicative_expression(TokenStream& stream, const VariableList& variables) const {
    auto result = parse_primary_expression(stream, variables);
    
    while(true){
        auto next_token = stream.peek_token();
        
        if(next_token.value != "*" && next_token.value != "/"){
            break;
        }
        
        stream.ignore_token(next_token);
        
        auto next_expression = parse_primary_expression(stream, variables);
        
        if(next_token.value == "*"){
            result = std::make_unique<MultiplicationNode>(std::move(result), std::move(next_expression));
        } else {
            result = std::make_unique<DivisionNode>(std::move(result), std::move(next_expression));
        }
    }
    
    return result;
}

std::unique_ptr<ExpressionNode> Table::parse_additive_expression(TokenStream& stream, const VariableList& variables) const {
    auto result = parse_multiplicative_expression(stream, variables);
    
    // cannot be parsed recursively because of left-to right operation order
    while(true){
        auto next_token = stream.peek_token();
        
        if(next_token.value != "-" && next_token.value != "+"){
            break;
        }
        
        stream.ignore_token(next_token);
        
        auto next_expression = parse_multiplicative_expression(stream, variables);
        
        if(next_token.value == "-"){
            result = std::make_unique<SubtractionNode>(std::move(result), std::move(next_expression));
        } else {
            result = std::make_unique<AdditionNode>(std::move(result), std::move(next_expression));
        }
    }
    
    return result;
}

CellVector Table::evaluate_expression(TokenStream& stream, const VariableList& variables) const{
    auto tree = parse_additive_expression(stream, variables);
    
    CellVector result(rows.size());
    for(size_t row_index = 0; row_index < rows.size(); ++row_index){
        const TableRow& row = rows[row_index];
        
        BoundRow row_ref(header, row);
        
        auto new_variables = variables + row_ref;
        
        result[row_index] = tree->evaluate(new_variables);
    }
    
    return result;
}


void Table::filter_by_condition(TokenStream& stream, const VariableList& variables, 
        std::function<Table(TokenStream&, const VariableList&)> select_callback) {
            
    auto lock = std::unique_lock(mutex);
    
    ConditionEvaluation evaluation(*this, stream, variables, select_callback);
    auto condition_result = evaluation.evaluate();
    
    std::vector<TableRow> new_rows;
    
    for(size_t i = 0; i < rows.size(); ++i){
        if(condition_result[i]){
            new_rows.push_back(std::move(rows[i]));
        }
    }
    
    rows = std::move(new_rows);
}