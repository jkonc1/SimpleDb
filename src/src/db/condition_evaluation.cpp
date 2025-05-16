#include "db/condition_evaluation.h"
#include "helper/like.h"
#include "helper/read_array.h"
#include "parse/token_to_cell.h"
#include "db/exceptions.h"

template<class Pred, class F, class ... C>
static BoolVector apply_condition(const Pred& condition, const F& first, const C&... others){
    BoolVector result(first.size());
    
    for(size_t i = 0; i < first.size(); ++i){
        result[i] = condition(first[i], others[i]...);
    }
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_exists(){
    stream.ignore_token("(");
    
    auto select_result = process_select();
    
    BoolVector result = apply_condition([](const Table& table){
        return table.row_count() > 0;
    }, select_result);
    
    stream.ignore_token(")");
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_inner_condition(){
    bool negated = stream.try_ignore_token("NOT");
    
    BoolVector result = evaluate_primary_condition();
    
    if(negated){
        result = !result;
    }
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_is(CellVector expression){
    bool is_negated = stream.try_ignore_token("NOT");
    
    stream.ignore_token("NULL");
    
    BoolVector result =  apply_condition([](const Cell& cell){
        return cell.type() == Cell::DataType::Null;
    }, expression);
    
    if(is_negated){
        result = !result;
    }
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_like(CellVector expression){
    auto pattern = stream.get_token(TokenType::String);
    
    return apply_condition([pattern](const Cell& cell){
        auto string = cell.repr();
        if(!string.has_value()){
            // NULL is not like anything
            return false;
        }
        return is_like(string.value(), pattern);
    }, expression);
}

BoolVector ConditionEvaluation::evaluate_in(CellVector expression){
    stream.ignore_token("(");
    
    std::vector<std::vector<Cell>> searched_values;
    
    if(stream.peek_token().like("SELECT")){
        searched_values = process_select_vectors();
    }
    else{
        auto tokens_in_array = read_array(stream);
        
        std::vector<Cell> cells_in_array;
        
        for(auto& token : tokens_in_array){
            cells_in_array.push_back(parse_token_to_cell(token));
        }
        
        // searched values are same for all rows
        searched_values = std::vector(expression.size(), cells_in_array);
    }
    
    stream.ignore_token(")");
    
    BoolVector result = apply_condition([](const Cell& target, const std::vector<Cell>& values){
        return std::count(std::begin(values), std::end(values), target) > 0;
    }, expression, searched_values);
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_between(CellVector expression){
    auto [left_type, left_side_expressions] = table.evaluate_expression(stream, variables);
    
    stream.ignore_token("AND");
    
    auto [right_type, right_side_expressions] = table.evaluate_expression(stream, variables);
    
    BoolVector result = (left_side_expressions <= expression) && (expression <= right_side_expressions);
    
    return result;
}

ConditionEvaluation::Comparator<Cell> ConditionEvaluation::operator_token_to_comparator(const Token& token){
    static const std::map<std::string, Comparator<Cell>> operator_to_comparator = {
        {"<", std::less<Cell>()},
        {"=", std::equal_to<Cell>()},
        {">", std::greater<Cell>()},
        {"<=", std::less_equal<Cell>()},
        {">=", std::greater_equal<Cell>()},
        {"<>", std::not_equal_to<Cell>()}
    };
    
    if(!operator_to_comparator.contains(token.value)){
        throw InvalidQuery("Invalid operator " + token.value);
    }
    
    return operator_to_comparator.at(token.value);
}

BoolVector ConditionEvaluation::evaluate_compare_subquery(CellVector expression, Comparator<Cell> comparator, bool has_any, bool has_all){
    stream.ignore_token("(");
    
    if(has_any && has_all){
        throw InvalidQuery("Cannot use ANY and ALL together");
    }
    
    if(!has_all && !has_any){
        auto query_result = process_select_singles();
        stream.ignore_token(")");
        
        return apply_condition(comparator, expression, query_result);
    }
    
    auto vectors = process_select_vectors();
    
    stream.ignore_token(")");
    
    return apply_condition([&comparator, has_any](const Cell& value, const std::vector<Cell>& possibilites){
        auto evaluations = apply_condition([value, &comparator](const Cell& possibility){
            return comparator(value, possibility);
        }, possibilites);
        
        if(has_any){
            // called with any
            return evaluations.max() == true;
        }
        
        else{
            // called with all
            return evaluations.min() == true;
        }
    }, expression, vectors);
}

BoolVector ConditionEvaluation::evaluate_compare(CellVector expression){
    Token comparator_token = stream.get_token();
    
    auto comparator = operator_token_to_comparator(comparator_token);
    
    bool has_any = stream.try_ignore_token("ANY");
    bool has_all = stream.try_ignore_token("ALL");
    
    if(stream.peek_token().like("(")){
        return evaluate_compare_subquery(std::move(expression), comparator, has_any, has_all);
    }
    
    auto [right_type, right_expression] = table.evaluate_expression(stream, variables);
    
    BoolVector result = apply_condition(comparator, expression, right_expression);
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_condition_switch(CellVector expression){
    if(stream.try_ignore_token("IS")){
        return evaluate_is(std::move(expression));
    }
    
    if(stream.try_ignore_token("LIKE")){
        return evaluate_like(std::move(expression));
    }
    
    if(stream.try_ignore_token("IN")){
        return evaluate_in(std::move(expression));
    }
    
    if(stream.try_ignore_token("BETWEEN")){
        return evaluate_between(std::move(expression));
    }
    
    return evaluate_compare(std::move(expression));
}

BoolVector ConditionEvaluation::evaluate_primary_condition(){
    if(stream.try_ignore_token("EXISTS")){
        return evaluate_exists();
    }
    
    bool bracketed = stream.try_ignore_token("(");
    
    auto [expr_type, expr] = table.evaluate_expression(stream, variables);
    
    if(bracketed){
        stream.ignore_token(")");
    }
    
    bool negated =  stream.try_ignore_token("NOT");
    
    BoolVector result = evaluate_condition_switch(std::move(expr));
    
    if(negated){
        result = !result;
    }
    
    return result;
    
}

BoolVector ConditionEvaluation::evaluate_conjunctive_condition(){
    auto result = evaluate_inner_condition();
    
    while(stream.try_ignore_token("AND")){
        auto right = evaluate_primary_condition();
        
        result = result && right;
    }
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_disjunctive_condition(){
    auto result = evaluate_conjunctive_condition();
    
    while(stream.try_ignore_token("OR")){
        auto right = evaluate_primary_condition();
        
        result = result || right;
    }
    
    return result;
}

BoolVector ConditionEvaluation::evaluate(){
    return evaluate_disjunctive_condition();
}

static std::string get_inside_brackets(TokenStream& stream){
    std::string result;
    
    size_t nesting_level = 1;
    
    while(true){
        auto next = stream.peek_token();
        
        if(next.like("(")){
            nesting_level++;
        }
        if(next.like(")")){
            nesting_level--;
        }
        
        if(nesting_level==0) { 
            break;
        }
        
        result += next.value + " ";
        stream.get_token();
    }
    
    return result;
}

Table ConditionEvaluation::process_select_single_row(const std::string& statement, const BoundRow& extra_vars){
    auto vars = variables + extra_vars;
    
    TokenStream new_stream(statement);
    
    return select_callback(new_stream, vars);
}

std::vector<Table> ConditionEvaluation::process_select(){
    std::string select_text = get_inside_brackets(stream);
    select_text += ";";
    
    std::vector<Table> result;
    
    for(auto&& row : table.rows){
        BoundRow bound(table.header, row);
        
        result.push_back(process_select_single_row(select_text, bound));
    }
    
    return result;
}

const Cell& ConditionEvaluation::extract_single_cell(const Table& table){
    if(table.get_columns().size() != 1){
        throw InvalidQuery("Subquery table must have 1 column");
    }
    if(table.row_count() != 1){
        throw InvalidQuery("Subquery table must have 1 row");
    }
    
    return table.rows[0][0];
}

std::vector<Cell> ConditionEvaluation::extract_vector(const Table& table){
    if(table.get_columns().size() != 1){
        throw InvalidQuery("Subquery table must have 1 column");
    }
    
    std::vector<Cell> result;
    
    for(auto&& row : table.rows){
        result.push_back(row[0]);
    }
    
    return result;
}

CellVector ConditionEvaluation::process_select_singles(){
    auto tables = process_select();
    
    CellVector result(tables.size());
    
    for(size_t i = 0; i < tables.size(); ++i){
        result[i] = extract_single_cell(tables[i]);
    }
    
    return result;
}

std::vector<std::vector<Cell>> ConditionEvaluation::process_select_vectors(){
    auto tables = process_select();
    
    std::vector<std::vector<Cell>> result(tables.size());
    
    for(size_t i = 0; i < tables.size(); ++i){
        result[i] = extract_vector(tables[i]);
    }
    
    return result;
}