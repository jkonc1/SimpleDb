#include "db/table.h"
#include "db/condition_evaluation.h"
#include "db/exceptions.h"
#include "db/expression.h"
#include "db/variable_list.h"
#include "parse/token_to_cell.h"
#include "helper/row_container.h"

#include <cassert>
#include <mutex>
#include <ranges>
#include <set>
#include <algorithm>
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
    // TODO: this is a bit weird with remade indexing, probably move to header
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
        stream.ignore_token("(");
        if(stream.try_ignore_token("*")){
            stream.ignore_token(")");
            return std::make_unique<ConstantNode>(Cell((int)row_count(), Cell::DataType::Int));
        }
        
        bool distinct = stream.try_ignore_token("DISTINCT");
        stream.try_ignore_token("ALL");
        
        std::string column = stream.get_token(TokenType::Identifier);
        
        stream.ignore_token(")");
        
        auto descriptor = header.get_column_info(column);
        
        if(!descriptor.has_value()){
            throw InvalidQuery("Unknown column " + column);
        }
        
        size_t column_index = descriptor->index;
        
        std::vector<Cell> cells_in_column;
        
        for(auto& row : rows){
            if(row[column_index].type() != Cell::DataType::Null){
                cells_in_column.push_back(row[column_index]);
            }
        }
        
        if(distinct){
            std::set<Cell> cell_set;
            for(auto& i : cells_in_column){
                cell_set.insert(std::move(i));
            }
            cells_in_column = std::vector<Cell>(cell_set.begin(), cell_set.end());
        }
        
        return std::make_unique<ConstantNode>(Cell((int)cells_in_column.size(), Cell::DataType::Int));
    }
    
    if(next_token.like("MAX") || next_token.like("MIN") || next_token.like("SUM") || next_token.like("AVG")){
        // TODO extract
         
        stream.ignore_token("(");
        
        bool is_distinct = stream.try_ignore_token("DISTINCT");
        
        auto [type, values] = evaluate_expression(stream, variables);
        
        if(values.size() == 0){
            return std::make_unique<ConstantNode>(Cell());
        }
        
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

std::pair<Cell::DataType, CellVector> Table::evaluate_expression(TokenStream& stream, const VariableList& variables) const{
    auto tree = parse_additive_expression(stream, variables);
    
    CellVector result(rows.size());
    for(size_t row_index = 0; row_index < rows.size(); ++row_index){
        const TableRow& row = rows[row_index];
        
        BoundRow row_ref(header, row);
        
        auto new_variables = variables + row_ref;
        
        result[row_index] = tree->evaluate(new_variables);
    }
    
    BoundRow dummy_row(header, TableRow(header.column_count()));
    auto type = tree->get_type(variables + dummy_row);
    
    return {type, std::move(result)};
}


void Table::filter_by_condition(TokenStream& stream, const VariableList& variables, 
        std::function<Table(TokenStream&, const VariableList&)> select_callback) {
            
    auto lock = std::unique_lock(mutex);
    
    auto condition_result = evaluate_condition(stream, variables, select_callback);
    
    std::vector<TableRow> new_rows;
    
    for(size_t i = 0; i < rows.size(); ++i){
        if(condition_result[i]){
            new_rows.push_back(std::move(rows[i]));
        }
    }
    
    rows = std::move(new_rows);
}

std::vector<Table> Table::group_by([[maybe_unused]] const std::vector<std::string>& grouping_columns) {
    std::unordered_map<std::vector<Cell>, Table, TableRowHash, TableRowIdentical> mapping;
    
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
    
    if(row_count() == 0){
        return false;
    }
    
    auto values = evaluate_condition(stream, variables, select_callback);
    
    return extract_same<BoolVector,bool>(values);
}

void Table::deduplicate(){
    std::unordered_set<TableRow, TableRowHash, TableRowIdentical> row_set;
    for(auto&& row : rows){
        row_set.insert(std::move(row));
    }
    
    rows = std::vector(row_set.begin(), row_set.end());
}

void Table::vertical_join(const Table& other){
    if(!(header.get_columns() == other.header.get_columns())){
        throw std::runtime_error("Attempt to join tables with different columns");
    }
    
    rows.append_range(other.rows);
}


Table Table::project(const std::vector<std::string>& expressions, const VariableList& variables) const {
    std::vector<std::pair<Cell::DataType, std::string>> column_definitions;
    std::vector<TableRow> new_table_rows(row_count());
    
    for(auto&& expr : expressions){
        TokenStream stream(expr);
        
        auto [type, values] = evaluate_expression(stream, variables);
        
        column_definitions.emplace_back(type, expr);
        
        for(size_t index = 0; index < row_count(); index++){
            new_table_rows[index].push_back(values[index]);
        }
    }
    
    Table result(std::move(column_definitions));
    result.rows = std::move(new_table_rows);
    
    return result;
}