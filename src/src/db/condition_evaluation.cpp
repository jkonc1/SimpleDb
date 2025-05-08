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

BoolVector ConditionEvaluation::evaluate_primary_condition(){
    if(stream.try_ignore_token("EXISTS")){
        stream.ignore_token("(");
        
        auto select_result = process_select();
        
        BoolVector result = apply_condition([](const Table& table){
            return table.row_count() > 0;
        }, select_result);
        
        stream.ignore_token(")");
        
        return result;
    }
    
    if(stream.try_ignore_token("(")){
        auto [type, values] = table.evaluate_expression(stream, variables);
        
        stream.ignore_token(")");
        stream.ignore_token("IS");
        
        bool is_negated = stream.try_ignore_token("NOT");
        
        stream.ignore_token("NULL");
        
        BoolVector result =  apply_condition([](const Cell& cell){
            return cell.type() == Cell::DataType::Null;
        }, values);
        
        if(is_negated){
            result = !result;
        }
        
        return result;
    }
    
    auto [expr_type, expr] = table.evaluate_expression(stream, variables);
    
    bool negated = stream.try_ignore_token("NOT");
    
    if(stream.try_ignore_token("LIKE")){
        auto pattern = stream.get_token(TokenType::String);
        
        BoolVector result = apply_condition([pattern](const Cell& cell){
            auto string = cell.repr();
            if(!string.has_value()){
                // NULL is not like anything
                return false;
            }
            return is_like(string.value(), pattern);
        }, expr);
        
        if(negated){
            result = !result;
        }
        
        return result;
    }
    
    if(stream.try_ignore_token("IN")){
        stream.ignore_token("(");
        
        std::vector<std::vector<Cell>> searched_values;
        
        if(stream.peek_token().like("SELECT")){
            searched_values = process_select_vectors();
            
        }
        else{
            auto tokens = read_array(stream);
            
            std::vector<Cell> converted;
            
            for(auto& token : tokens){
                converted.push_back(parse_token_to_cell(token));
            }
            
            searched_values = std::vector(expr.size(), converted);
        }
        
        BoolVector result = apply_condition([](const Cell& target, const std::vector<Cell>& values){
            return std::count(std::begin(values), std::end(values), target) > 0;
        }, expr, searched_values);
        
        if(negated){
            result = !result;
        }
        
        return result;
    }
    
    if(stream.try_ignore_token("BETWEEN")){
        auto [left_type, left_side] = table.evaluate_expression(stream, variables);
        
        stream.ignore_token("AND");
        
        auto [right_type, right_side] = table.evaluate_expression(stream, variables);
        
        BoolVector result = (left_side <= expr) && (expr <= right_side);
        
        if(negated){
            result = !result;
        }
        
        return result;
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
        BoolVector result;
        
        if(!has_all && !has_any){
            auto query_result = process_select_singles();
            
            result = apply_condition(comparator, expr, query_result);
        }
        
        else{
            auto vectors = process_select_vectors();
            
            result = apply_condition([&comparator, has_any](const Cell& value, const std::vector<Cell>& possibilites){
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
            }, expr, vectors);
        }
        
        stream.ignore_token(")");
        
        return result;
    }
    
    // here we just compare against an expression
    
    auto [right_type, right_expression] = table.evaluate_expression(stream, variables);
    
    BoolVector result = apply_condition(comparator, expr, right_expression);
    
    if(negated){
        result = !result;
    }
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_conjunctive_condition(){
    auto result = evaluate_primary_condition();
    
    while(stream.try_ignore_token("AND")){
        auto right = evaluate_primary_condition();
        
        result &= right;
    }
    
    return result;
}

BoolVector ConditionEvaluation::evaluate_disjunctive_condition(){
    auto result = evaluate_conjunctive_condition();
    
    while(stream.try_ignore_token("OR")){
        auto right = evaluate_primary_condition();
        
        result |= right;
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

const Cell& ConditionEvaluation::extract_single_cell(const Table& table){ //NOLINT needs friend access
    if(table.get_columns().size() != 1){
        throw InvalidQuery("Subquery table must have 1 column");
    }
    if(table.row_count() != 1){
        throw InvalidQuery("Subquery table must have 1 row");
    }
    
    return table.rows[0][0];
}

std::vector<Cell> ConditionEvaluation::extract_vector(const Table& table){ //NOLINT needs friend access
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