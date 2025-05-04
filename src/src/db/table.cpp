#include "db/table.h"
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
        
        size_t index = columns.size() - 1;
        
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

static std::vector<Cell> get_distinct(const std::vector<Cell>& values){
    std::set<Cell> values_set(values.begin(), values.end());
    return std::vector<Cell>(values_set.begin(), values_set.end());
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
            value = *std::max_element(values.begin(), values.end());
        } else if(next_token.like("MIN")){
            value = *std::min_element(values.begin(), values.end());
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

std::vector<Cell> Table::evaluate_expression(TokenStream& stream, const VariableList& variables) const{
    auto tree = parse_additive_expression(stream, variables);
    
    std::vector<Cell> result;
    for(const auto& row : rows){
        BoundRow row_ref(header, row);
        
        auto new_variables = variables + row_ref;
        
        result.push_back(tree->evaluate(new_variables));
    }
    
    return result;
}


static std::vector<bool> negate(const std::vector<bool>& values){
    std::vector<bool> result;
    for(auto value : values){
        result.push_back(!value);
    }
    return result;
}

std::vector<bool> Table::evaluate_primary_condition(TokenStream& stream, const VariableList& variables) const{
    if(stream.try_ignore_token("EXISTS")){
        stream.ignore_token("(");
        
        auto result = process_select(stream);
        
        stream.ignore_token(")");
        
        return result.row_count() > 0;
    }
    
    if(stream.try_ignore_token("(")){
        auto values = evaluate_expression(stream, variables);
        
        stream.ignore_token(")");
        stream.ignore_token("IS");
        
        bool is_negated = stream.try_ignore_token("NOT");
        
        stream.ignore_token("NULL");
        
        auto result = values | std::views::transform([is_negated](const Cell& cell){
            return is_negated == (cell.type() != Cell::DataType::Null);
        });
        
        return std::vector(result.begin(), result.end());
    }
    
    auto expr = evaluate_expression(stream, variables);
    
    bool negated = stream.try_ignore_token("NOT");
    
    if(stream.try_ignore_token("LIKE")){
        auto pattern = stream.get_token(TokenType::String);
        
        auto result = expr | std::views::transform([pattern, negated](const Cell& cell){
            auto repr = cell.repr();
            if(!repr.has_value()){
                return false;
            }
            return is_like(repr.value(), pattern) == !negated;
        });
        
        return std::vector(result.begin(), result.end());
    }
    
    if(stream.try_ignore_token("IN")){
        stream.ignore_token("(");
        
        if(stream.peek_token().like("SELECT")){
            Table table = process_select(stream);
            
            if(table.get_columns().size() != 1){
                throw std::runtime_error("IN clause expects a single column");
            }
            
            auto result = expr | std::views::transform([&table, negated](const Cell& cell){
                for(auto& row : table){
                    if(row[0] == cell){
                        return !negated;
                    }
                }
                return negated;
            });
            
        }
        
        auto values = read_array(stream);
        
        auto cells = values | std::views::transform([](const Token& token){
            return parse_token_to_cell(token);
        });
        
        auto result = expr | std::views::transform([cells, negated](const Cell& cell){
            bool found = std::find(cells.begin(), cells.end(), cell) != cells.end();
            return found == !negated;
        });
        
        return std::vector(result.begin(), result.end());
    }
    
    if(stream.try_ignore_token("BETWEEN")){
        auto left_side = evaluate_expression(stream, variables);
        
        stream.ignore_token("AND");
        
        auto right_side = evaluate_expression(stream, variables);
        
        auto result = std::views::zip(expr,left_side,right_side) | std::views::transform([negated](const Cell& left, const Cell& middle, const Cell& right){
            bool left_boundary = left <= middle;
            bool right_boundary = middle <= right;
            return (left_boundary && right_boundary) == !negated;
        });
        
        return std::vector(result.begin(), result.end());
    }
    
    
    //otherwise it must be one of the comparisions
    
    auto oper = stream.get_token();
    
    std::map<std::string, std::function<bool(const Cell&, const Cell&)>> operator_to_comparator = {{"<", std::less<Cell>()}, {"=", std::equal_to<Cell>()}, {">", std::greater<Cell>()}, {"<=", std::less_equal<Cell>()}, {">=", std::greater_equal<Cell>()}, {"<>", std::not_equal_to<Cell>()}};
    
    if(!operator_to_comparator.contains(oper.value)){
        throw InvalidQuery("Invalid operator " + oper.value);
    }
    
    auto comparator = operator_to_comparator.at(oper.value);
    
    bool has_any = stream.try_ignore_token("ANY");
    bool has_all = stream.try_ignore_token("ALL");
    
    if(has_any && has_all){
        throw InvalidQuery("Cannot use ANY and ALL together");
    }
    
    if(stream.try_ignore_token("(")){
        std::vector<bool> result;
        
        if(!has_all && !has_any){
            // here the select must return a single value and we compare to it
            // TODO fix stream copying
           result = expr | std::views::transform([comparator, has_all, has_any, &stream](const Cell& value){
                Cell query_result = process_single_value_query(stream);
                return comparator(value, query_result);
            });
        }
        
        // here the select must return a vector of values and we compare each to the value
        // TODO fix stream copying
        else{
            auto result = expr | std::views::transform([comparator, has_all, has_any, &stream](const Cell& value){
                std::vector<Cell> query_results = process_vector_query(stream);
                
                std::vector<bool> comparisons;
                for(const auto& query_result : query_results){
                    comparisons.push_back(comparator(value, query_result));
                }
                
                if(has_any){
                    return std::count(comparisons.begin(), comparisons.end(), true) > 0;
                }
                
                return std::count(comparisons.begin(), comparisons.end(), false) == 0;
            });
        }
        
        stream.ignore_token(")");
        
        return result;
    }
    
    // here we just compare against an expression
    
    auto right_expression = evaluate_expression(stream, variables);
    
    std::vector<bool> result;
    
    for(size_t i = 0; i < expr.size(); ++i){
        result.push_back(comparator(expr[i], right_expression[i]));
    }
    
    return result;
}

std::vector<bool> Table::evaluate_conjunctive_condition(TokenStream& stream, const VariableList& variables) const{
    auto result = evaluate_primary_condition(stream, variables);
    
    while(stream.try_ignore_token("AND")){
        auto right = evaluate_primary_condition(stream, variables);
        
        for(auto&& field : result){
            field = field && right[field];
        }
    }
    
    return result;
}

std::vector<bool> Table::evaluate_disjunctive_condition(TokenStream& stream, const VariableList& variables) const{
    auto result = evaluate_conjunctive_condition(stream, variables);
    
    while(stream.try_ignore_token("OR")){
        auto right = evaluate_primary_condition(stream, variables);
        
        for(auto&& field : result){
            field = field || right[field];
        }
    }
    
    return result;
}

std::vector<bool> Table::evaluate_condition(TokenStream& stream, const VariableList& variables) const{
    return evaluate_disjunctive_condition(stream, variables);
}

void Table::filter_by_condition(TokenStream& stream, const VariableList& variables){
    auto lock = std::unique_lock(mutex);
    
    auto condition = evaluate_condition(stream, variables);
    
    std::vector<TableRow> new_rows;
    
    for(size_t i = 0; i < rows.size(); ++i){
        if(condition[i]){
            new_rows.push_back(std::move(rows[i]));
        }
    }
    
    rows = std::move(new_rows);
}