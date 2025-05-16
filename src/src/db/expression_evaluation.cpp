#include "db/expression_evaluation.h"
#include "parse/token_to_cell.h"

#include <set>
#include <algorithm>

template <class C>
static C get_distinct(const C& values){
    std::set values_set(std::begin(values), std::end(values));
    
    C result(values_set.size());
    
    std::copy(std::begin(values_set), std::end(values_set), std::begin(result));
    
    return result;
}

static bool is_aggregate(const Token& token){
    static const std::vector<std::string> aggregates = {"MIN", "MAX", "SUM", "AVG"};

    return std::ranges::any_of(aggregates, [&](const std::string& agg) {
        return token.like(agg);
    });
}

std::unique_ptr<ExpressionNode> ExpressionEvaluation::parse_count() {
    stream.ignore_token("(");
    
    if(stream.try_ignore_token("*")){
        stream.ignore_token(")");
        return std::make_unique<ConstantNode>(Cell((int)table.row_count(), Cell::DataType::Int));
    }
    
    bool distinct = stream.try_ignore_token("DISTINCT");
    stream.try_ignore_token("ALL"); // ALL is the default so does nothing
    std::string column = stream.get_token(TokenType::Identifier);
    stream.ignore_token(")");
    
    auto descriptor = table.header.get_column_info(column);
    if(!descriptor.has_value()){
        throw InvalidQuery("Unknown column " + column);
    }
    size_t column_index = descriptor->index;
    
    std::vector<Cell> cells_in_column;
    
    for(auto&& row : table.rows){
        if(row[column_index].type() != Cell::DataType::Null){
            cells_in_column.push_back(row[column_index]);
        }
    }
    
    if(distinct){
        cells_in_column=get_distinct(cells_in_column);
    }
    
    return std::make_unique<ConstantNode>(Cell((int)cells_in_column.size(), Cell::DataType::Int));
}

std::unique_ptr<ExpressionNode> ExpressionEvaluation::parse_aggregate(){
    Token aggregate_type = stream.get_token();
    
    stream.ignore_token("(");
    
    bool is_distinct = stream.try_ignore_token("DISTINCT");
    
    auto [column_type, column_values] = table.evaluate_expression(stream, variables);
    stream.ignore_token(")");
    
    if(column_values.size() == 0){
        return std::make_unique<ConstantNode>(Cell());
    }
    
    if(is_distinct){
        column_values = get_distinct(column_values);
    }
    
    
    if(aggregate_type.like("MAX")){
        return std::make_unique<ConstantNode>(column_values.max());
    }
    if(aggregate_type.like("MIN")){
        return std::make_unique<ConstantNode>(column_values.min());
    }
    
    
    Cell result = column_values[0];
    for(size_t i = 1; i < column_values.size(); ++i){
        result += column_values[i];
    }
    
    if(aggregate_type.like("AVG")){
        result /= Cell((int)column_values.size(), Cell::DataType::Int);
    }
    
    return std::make_unique<ConstantNode>(result);
}

std::unique_ptr<ExpressionNode> ExpressionEvaluation::parse_primary_expression() {
    
    const Token& next_token = stream.peek_token();
    
    if(stream.try_ignore_token("NULL")){
        stream.get_token();
        return std::make_unique<ConstantNode>(Cell());
    }
    
    if(stream.try_ignore_token("COUNT")){
        return parse_count();
    }
    
    if(is_aggregate(next_token)){
        return parse_aggregate();
    }
    
    if(next_token.type == TokenType::Identifier){
        // column name
        std::string column_name = stream.get_token(TokenType::Identifier);
        
        while(stream.try_ignore_token(".")){
            column_name += "." + stream.get_token(TokenType::Identifier);
        }
        
        return std::make_unique<VariableNode>(column_name);
    }
    
    Token token = stream.get_token();
    // just a constant
    Cell value = parse_token_to_cell(token);
    
    return std::make_unique<ConstantNode>(value);
    
}

std::unique_ptr<ExpressionNode> ExpressionEvaluation::parse_multiplicative_expression() {
    auto result = parse_primary_expression();
    
    while(true){
        auto next_token = stream.peek_token();
        
        if(next_token.value != "*" && next_token.value != "/"){
            break;
        }
        
        stream.ignore_token(next_token);
        
        auto next_expression = parse_primary_expression();
        
        if(next_token.value == "*"){
            result = std::make_unique<MultiplicationNode>(std::move(result), std::move(next_expression));
        } else {
            result = std::make_unique<DivisionNode>(std::move(result), std::move(next_expression));
        }
    }
    
    return result;
}

std::unique_ptr<ExpressionNode> ExpressionEvaluation::parse_additive_expression() {
    auto result = parse_multiplicative_expression();
    
    // cannot be parsed recursively because of left-to right operation order
    while(true){
        auto next_token = stream.peek_token();
        
        if(next_token.value != "-" && next_token.value != "+"){
            break;
        }
        
        stream.ignore_token(next_token);
        
        auto next_expression = parse_multiplicative_expression();
        
        if(next_token.value == "-"){
            result = std::make_unique<SubtractionNode>(std::move(result), std::move(next_expression));
        } else {
            result = std::make_unique<AdditionNode>(std::move(result), std::move(next_expression));
        }
    }
    
    return result;
}

EvaluatedExpression ExpressionEvaluation::evaluate() {
    auto tree = parse_additive_expression();
    
    CellVector result(table.rows.size());
    for(size_t row_index = 0; row_index < table.rows.size(); ++row_index){
        const TableRow& row = table.rows[row_index];
        
        BoundRow row_ref(table.header, row);
        
        auto new_variables = variables + row_ref;
        
        result[row_index] = tree->evaluate(new_variables);
    }
    
    BoundRow dummy_row(table.header, TableRow(table.header.column_count()));
    auto type = tree->get_type(variables + dummy_row);
    
    return {type, std::move(result)};
}