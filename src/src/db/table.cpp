#include "db/table.h"
#include "db/condition_evaluation.h"
#include "db/expression_evaluation.h"
#include "db/exceptions.h"
#include "db/expression.h"
#include "db/variable_list.h"
#include "helper/row_container.h"

#include <cassert>
#include <mutex>
#include <ranges>
#include <functional>
#include <unordered_set>

TableHeader::TableHeader(std::vector<std::pair<Cell::DataType, std::string>> column_definitions)
{
    for(auto&& [column_def, index] : std::views::zip(column_definitions, std::views::iota((size_t)0))){
        auto&& [column_type, column_name] = column_def;
        ColumnDescriptor column = {.alias = "", .name = std::move(column_name), .type = column_type, .index=index};
        
        columns.push_back(column);
        
    }
    
    calculate_lookup_map();
}

TableHeader TableHeader::join(const TableHeader& left, const TableHeader& right){
    return TableHeader(left, right);
}

TableHeader TableHeader::add_alias(const std::string& alias) const {
    TableHeader header(*this);
    
    for(auto& column : header.columns){
        column.alias = alias;
        
        std::string qualified_name = alias + "." + column.name;
        
        header.column_to_index.emplace(qualified_name, column.index);
    }
    
    return header;
}

std::optional<ColumnDescriptor> TableHeader::get_column_info(const std::string& name) const {
    if(!column_to_index.contains(name)){
        return std::nullopt;
    }
    
    if(column_to_index.count(name) > 1){
        throw InvalidQuery("Ambiguous column access : " + name);
    }
    
    size_t index = column_to_index.find(name) -> second;
    
    return columns[index];
}

TableHeader::TableHeader(const TableHeader& left, const TableHeader& right) :
    columns(left.columns), column_to_index(left.column_to_index){
    
    for(auto&& column : right.columns){
        ColumnDescriptor column_copy(column);
        
        size_t index = columns.size();
        
        column_copy.index = index;
        
        columns.push_back(column_copy);
    }
    
    calculate_lookup_map();
}

void TableHeader::calculate_lookup_map(){
    column_to_index.clear();
    for(auto column : columns){
        column_to_index.emplace(column.name, column.index);
        
        if(!column.alias.empty()){
            std::string qualified_name = column.alias + "." + column.name;
            
            column_to_index.emplace(qualified_name, column.index);
        }
    }
}

TableRow TableHeader::create_row(const std::map<std::string, std::string>& data) const {
    std::vector<Cell> cells(column_count());
    
    for(auto&& [column_name, value] : data){
        auto column = get_column_info(column_name);
        
        if(!column.has_value()){
            throw InvalidQuery("Column '" + column_name + "' does not exist");
        }
        
        size_t index = column.value().index;
        
        cells[index] = Cell(value, columns[index].type);
    }
    
    return cells;
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

Table::Table(std::vector<std::pair<Cell::DataType, std::string>> columns):
    Table(TableHeader(std::move(columns)))
{
}

Table::Table(Table&& other) noexcept : 
    header(std::move(other.header)), rows(std::move(other.rows))
{
}

Table& Table::operator=(Table&& other) noexcept {
    header = std::move(other.header);
    rows = std::move(other.rows);
    
    return *this;
}

void Table::add_row(TableRow data){
    rows.push_back(std::move(data));
}

void Table::add_row(const std::vector<std::string>& data){
    auto lock = std::unique_lock(mutex);
    
    if(data.size() != get_columns().size()){
        throw InvalidQuery("Wrong number of inserted fields");
    }
    
    std::vector<Cell> converted_data;
    for(auto&& [value, column] : std::views::zip(data, get_columns())){
        converted_data.push_back(Cell(value, column.type));
    }
    
    add_row(std::move(converted_data));
}

void Table::add_row(const std::map<std::string, std::string>& values){
    auto lock = std::unique_lock(mutex);
    
    add_row(header.create_row(values));
}

Table Table::cross_product(std::vector<std::pair<const Table&, std::string>> tables){
    assert(!tables.empty());
    
    std::vector<std::shared_lock<std::shared_mutex>> locks;
    for(auto& [table, alias] : tables){
        locks.emplace_back(table.mutex);
    }
    
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


void Table::filter_by_condition(TokenStream& stream, const VariableList& variables, 
        std::function<Table(TokenStream&, const VariableList&)> select_callback, bool negate) {
            
    auto condition_result = evaluate_condition(stream, variables, select_callback);
    
    auto lock = std::unique_lock(mutex);
    
    std::vector<TableRow> new_rows;
    
    for(size_t i = 0; i < rows.size(); ++i){
        if(condition_result[i] != negate){
            new_rows.push_back(std::move(rows[i]));
        }
    }
    
    rows = std::move(new_rows);
}

std::vector<Table> Table::group_by(const std::vector<std::string>& grouping_columns) {
    std::unordered_map<std::vector<Cell>, Table, TableRowHash, TableRowIdentical> mapping;

    auto lock = std::unique_lock(mutex);
    
    std::vector<size_t> selected_column_indexes;
    
    for(auto&& column : grouping_columns){
        auto descriptor = header.get_column_info(column);
        
        if(!descriptor.has_value()){
            throw InvalidQuery("Grouping by non-existent column " + column);
        }
        
        selected_column_indexes.push_back(descriptor->index);
    }
    
    for(auto&& row : rows){
        std::vector<Cell> selected_columns;
        for(size_t i : selected_column_indexes){
            selected_columns.push_back(row[i]);
        }
        
        mapping.try_emplace(selected_columns, header);
        
        mapping.at(selected_columns).add_row(std::move(row));
    }
    
    rows.clear();
    
    std::vector<Table> result;
    
    for(auto&& [groups, table] : mapping){
        result.push_back(std::move(table));
    }
    
    return result;
}

BoolVector Table::evaluate_condition(TokenStream& stream, const VariableList& variables, 
        std::function<Table(TokenStream&, const VariableList&)> select_callback) const {
    
    ConditionEvaluation evaluation(*this, stream, variables, select_callback);
    return  evaluation.evaluate();
}

EvaluatedExpression Table::evaluate_expression(TokenStream& stream, const VariableList& variables) const {
    ExpressionEvaluation expr(*this, stream, variables);
    
    return expr.evaluate();
}

template<class C, class T>
static T extract_same(const C& container){
    T first = container[0];
    
    for(const auto& i : container){
        if(i!=first){
            throw InvalidQuery("Non aggregate used as aggregate");
        }
    }
    
    return first;
}

bool Table::evaluate_aggregate_condition(TokenStream& stream, const VariableList& variables, 
        std::function<Table(TokenStream&, const VariableList&)> select_callback) const {
    
    auto lock = std::shared_lock(mutex);
    
    if(row_count() == 0){
        return false;
    }
    
    auto values = evaluate_condition(stream, variables, select_callback);
    
    return extract_same<BoolVector,bool>(values);
}

void Table::deduplicate(){
    auto lock = std::unique_lock(mutex);
    
    std::unordered_set<TableRow, TableRowHash, TableRowIdentical> row_set;
    for(auto&& row : rows){
        row_set.insert(std::move(row));
    }
    
    rows = std::vector(row_set.begin(), row_set.end());
}

void Table::vertical_join(const Table& other){
    auto lock = std::unique_lock(mutex);
    
    rows.append_range(other.rows);
}

Table Table::project(const std::vector<std::string>& expressions, const VariableList& variables, bool aggregate_mode) const {
    auto lock = std::shared_lock(mutex);
    
    if(expressions == std::vector<std::string>{"*"} && !aggregate_mode){
        return clone();
    }
    
    std::vector<std::pair<Cell::DataType, std::string>> column_definitions;
    std::vector<TableRow> new_table_rows;
    
    if (aggregate_mode) {
        new_table_rows.resize(1);
    } else {
        new_table_rows.resize(row_count());
    }
    
    for(auto&& expr : expressions){
        TokenStream stream(expr);
        
        auto [type, values] = evaluate_expression(stream, variables);
        
        column_definitions.emplace_back(type, expr);
        
        if (aggregate_mode) {
            if (values.size() > 0) {
                new_table_rows[0].push_back(values[0]);
            } else {
                if (expr.find("COUNT") != std::string::npos) {
                    new_table_rows[0].push_back(Cell(0, Cell::DataType::Int));
                } else {
                    new_table_rows[0].push_back(Cell()); 
                }
            }
        } else {
            for(size_t index = 0; index < row_count(); index++){
                new_table_rows[index].push_back(values[index]);
            }
        }
    }
    
    Table result(std::move(column_definitions));
    result.rows = std::move(new_table_rows);
    
    return result;
}

Table Table::clone() const{
    Table result(header);
    
    result.rows = (std::vector<TableRow>)rows;
    
    return result;
}