#ifndef CONDITION_EVALUATION_H
#define CONDITION_EVALUATION_H

#include "db/table.h"
#include "db/variable_list.h"

#include <vector>
#include <functional>

class ConditionEvaluation{
public:
    ConditionEvaluation(const Table& table, TokenStream& stream, const VariableList& variables, 
        std::function<Table(TokenStream&, const VariableList&)>& select_callback) : 
            table(table), stream(stream), variables(variables), select_callback(select_callback) {}
        
    BoolVector evaluate();
private:
    const Table& table;
    TokenStream& stream;
    const VariableList& variables;
    std::function<Table(TokenStream&, const VariableList&)>& select_callback;
    
    template<class T>
    using Comparator = std::function<bool(const T&, const T&)>;
        
    BoolVector evaluate_conjunctive_condition();
    BoolVector evaluate_disjunctive_condition();
    BoolVector evaluate_primary_condition();
    BoolVector evaluate_inner_condition();
    
    BoolVector evaluate_condition_switch(CellVector expression);
    BoolVector evaluate_exists();
    BoolVector evaluate_is(CellVector expression);
    BoolVector evaluate_in(CellVector expression);
    BoolVector evaluate_between(CellVector expression);
    BoolVector evaluate_like(CellVector expression);
    BoolVector evaluate_compare(CellVector expression);
    BoolVector evaluate_compare_subquery(CellVector expression, Comparator<Cell> comparator, bool has_any, bool has_all);
    
    static Comparator<Cell> operator_token_to_comparator(const Token& token);
    
    Table process_select_single_row(const std::string& statement, const BoundRow& extra_vars);
    
    
    std::vector<Table> process_select();
    
    CellVector process_select_singles();
    std::vector<std::vector<Cell>> process_select_vectors();
    
    const Cell& extract_single_cell(const Table& table);
    std::vector<Cell> extract_vector(const Table& table);
};

#endif